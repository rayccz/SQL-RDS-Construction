#ifndef _qe_h_
#define _qe_h_

#include <vector>

#include "../rbf/rbfm.h"
#include "../rm/rm.h"
#include "../ix/ix.h"

#define QE_EOF (-1)  // end of the index scan

using namespace std;

typedef enum{ MIN=0, MAX, COUNT, SUM, AVG } AggregateOp;

// The following functions use the following
// format for the passed data.
//    For INT and REAL: use 4 bytes
//    For VARCHAR: use 4 bytes for the length followed by the characters

struct Value {
    AttrType type;          // type of value
    void     *data;         // value
};


struct Condition {
    string  lhsAttr;        // left-hand side attribute
    CompOp  op;             // comparison operator
    bool    bRhsIsAttr;     // TRUE if right-hand side is an attribute and not a value; FALSE, otherwise.
    string  rhsAttr;        // right-hand side attribute if bRhsIsAttr = TRUE
    Value   rhsValue;       // right-hand side value if bRhsIsAttr = FALSE
};


class Iterator {
    // All the relational operators and access methods are iterators.
    public:
        virtual RC getNextTuple(void *data) = 0;
        virtual void getAttributes(vector<Attribute> &attrs) const = 0;
        virtual ~Iterator() {};
};


class TableScan : public Iterator
{
    // A wrapper inheriting Iterator over RM_ScanIterator
    public:
        RelationManager &rm;
        RM_ScanIterator *iter;
        string tableName;
        vector<Attribute> attrs;
        vector<string> attrNames;
        RID rid;

        TableScan(RelationManager &rm, const string &tableName, const char *alias = NULL):rm(rm)
        {
			//Set members
			this->tableName = tableName;

            // Get Attributes from RM
            rm.getAttributes(tableName, attrs);

            // Get Attribute Names from RM
            unsigned i;
            for(i = 0; i < attrs.size(); ++i)
            {
                // convert to char *
                attrNames.push_back(attrs.at(i).name);
            }

            // Call RM scan to get an iterator
            iter = new RM_ScanIterator();
            rm.scan(tableName, "", NO_OP, NULL, attrNames, *iter);

            // Set alias
            if(alias) this->tableName = alias;
        };

        // Start a new iterator given the new compOp and value
        void setIterator()
        {
            iter->close();
            delete iter;
            iter = new RM_ScanIterator();
            rm.scan(tableName, "", NO_OP, NULL, attrNames, *iter);
        };

        RC getNextTuple(void *data)
        {
            return iter->getNextTuple(rid, data);
        };

        void getAttributes(vector<Attribute> &attrs) const
        {
            attrs.clear();
            attrs = this->attrs;
            unsigned i;

            // For attribute in vector<Attribute>, name it as rel.attr
            for(i = 0; i < attrs.size(); ++i)
            {
                string tmp = tableName;
                tmp += ".";
                tmp += attrs.at(i).name;
                attrs.at(i).name = tmp;
            }
        };

        ~TableScan()
        {
        	iter->close();
        };
};


class IndexScan : public Iterator
{
    // A wrapper inheriting Iterator over IX_IndexScan
    public:
        RelationManager &rm;
        RM_IndexScanIterator *iter;
        string tableName;
        string attrName;
        vector<Attribute> attrs;
        char key[PAGE_SIZE];
        RID rid;

        IndexScan(RelationManager &rm, const string &tableName, const string &attrName, const char *alias = NULL):rm(rm)
        {
			// Set members
			this->tableName = tableName;
			this->attrName = attrName;

            // Get Attributes from RM
            rm.getAttributes(tableName, attrs);

            // Call rm indexScan to get iterator
            iter = new RM_IndexScanIterator();
            rm.indexScan(tableName, attrName, NULL, NULL, true, true, *iter);

            // Set alias
            if(alias) this->tableName = alias;
        };

        // Start a new iterator given the new key range
        void setIterator(void* lowKey,
                         void* highKey,
                         bool lowKeyInclusive,
                         bool highKeyInclusive)
        {
            iter->close();
            delete iter;
            iter = new RM_IndexScanIterator();
            rm.indexScan(tableName, attrName, lowKey, highKey, lowKeyInclusive,
                           highKeyInclusive, *iter);
        };

        RC getNextTuple(void *data)
        {
            int rc = iter->getNextEntry(rid, key);

            if(rc == 0)
            {
            		rc = rm.readTuple(tableName.c_str(), rid, data);
            }
            return rc;
        };

        void getAttributes(vector<Attribute> &attrs) const
        {
            attrs.clear();
            attrs = this->attrs;
            unsigned i;

            // For attribute in vector<Attribute>, name it as rel.attr
            for(i = 0; i < attrs.size(); ++i)
            {
                string tmp = tableName;
                tmp += ".";
                tmp += attrs.at(i).name;
                attrs.at(i).name = tmp;
            }
        };

        ~IndexScan()
        {
            iter->close();
        };
};


class Filter : public Iterator {
	Iterator *input;
	Condition condition;
	vector<Attribute> attrs;
	void* leftValue;
	void* rightValue;

    // Filter operator
    public:
        Filter(Iterator *input,               // Iterator of input R
               const Condition &condition     // Selection condition
        );
        ~Filter(){
        		free(leftValue);
        		free(rightValue);
        };

        bool compare(AttrType type, CompOp op){
        		if(type == TypeInt){
        			int leftValue_int;
        			int rightValue_int;
        			memcpy(&leftValue_int, leftValue, sizeof(int));
        			memcpy(&rightValue_int, rightValue, sizeof(int));

//        			cout << "left:" << leftValue_int << "  right: " << rightValue_int << endl;
        			// compare
        			if(op == EQ_OP)			return (leftValue_int == rightValue_int);
        			else if(op == LT_OP)		return (leftValue_int < rightValue_int);
        			else if(op == LE_OP)		return (leftValue_int <= rightValue_int);
        			else if(op == GT_OP)		return (leftValue_int > rightValue_int);
        			else if(op == GE_OP)		return (leftValue_int >= rightValue_int);
        			else if(op == NE_OP)		return (leftValue_int != rightValue_int);
        		}else if(type == TypeReal){
        			float leftValue_float;
        			float rightValue_float;
        			memcpy(&leftValue_float, leftValue, sizeof(float));
        			memcpy(&rightValue_float, rightValue, sizeof(float));

        			// compare
        			if(op == EQ_OP)			return (leftValue_float == rightValue_float);
        			else if(op == LT_OP)		return (leftValue_float < rightValue_float);
        			else if(op == LE_OP)		return (leftValue_float <= rightValue_float);
        			else if(op == GT_OP)		return (leftValue_float > rightValue_float);
        			else if(op == GE_OP)		return (leftValue_float >= rightValue_float);
        			else if(op == NE_OP)		return (leftValue_float != rightValue_float);
        		}else if(type == TypeVarChar){
        			int leftValue_length;
        			int rightValue_length;
        			memcpy(&leftValue_length, leftValue, sizeof(int));
        			memcpy(&rightValue_length, rightValue, sizeof(int));

        			void* leftValue_string = malloc(leftValue_length);
				void* rightValue_string = malloc(rightValue_length);
				memcpy(leftValue_string, ((char*)leftValue) + sizeof(int), leftValue_length);
				memcpy(rightValue_string, ((char*)rightValue) + sizeof(int), rightValue_length);

//				cout << "leftValue_length:" << leftValue_length << endl;
//				cout << "rightValue_length:" << rightValue_length << endl;
//				cout << "leftValue_string:" << *((char*)leftValue_string) << endl;
//				cout << "rightValue_string:" << *((char*)rightValue_string) << endl;

				// compare
				if(op == EQ_OP)			return (*((char*)leftValue_string) == *((char*)rightValue_string));
				else if(op == LT_OP)		return (*((char*)leftValue_string) < *((char*)rightValue_string));
				else if(op == LE_OP)		return (*((char*)leftValue_string) <= *((char*)rightValue_string));
				else if(op == GT_OP)		return (*((char*)leftValue_string) > *((char*)rightValue_string));
				else if(op == GE_OP)		return (*((char*)leftValue_string) >= *((char*)rightValue_string));
				else if(op == NE_OP)		return (*((char*)leftValue_string) != *((char*)rightValue_string));
				free(leftValue_string);
				free(rightValue_string);
        		}

        		return -99;
        };

        RC getNextTuple(void *data) {
        		while(input->getNextTuple(data) != QE_EOF){
        			// check right hand side is value or attribute
        			if(condition.bRhsIsAttr){
        				int offset = 0;
        				int nullFieldsIndicatorActualSize = ceil((double) attrs.size() / CHAR_BIT);
        				offset += nullFieldsIndicatorActualSize;

        				AttrType attrType;

        				// try to find leftValue and rightValue
        				for(int i = 0; i < attrs.size();i ++){
        					//get leftValue
        					if(attrs[i].name == condition.lhsAttr){
        						if(attrs[i].type == TypeVarChar){
        							int length;
        							memcpy(&length, ((char*)data) + offset, sizeof(int));
        							offset += sizeof(int);
        							//leftValue = malloc(length);
        							//memcpy(leftValue, ((char*)data) + offset, length);
        							memcpy(leftValue, ((char*)data) + offset - 4, length + 4); //keep value has format (length)(varchar)
        							offset += length;
        						}else{
        							//leftValue = malloc(4);
        							memcpy(leftValue, ((char*)data) + offset, 4);
        							offset += 4; //float or int
        						}
        						//find, continue
        						attrType = attrs[i].type;
        						continue;
        					}

        					//get rightValue
        					if(attrs[i].name == condition.rhsAttr){
        						if(attrs[i].type == TypeVarChar){
								int length;
								memcpy(&length, ((char*)data) + offset, sizeof(int));
								offset += sizeof(int);
								//rightValue = malloc(length);

								//memcpy(rightValue, ((char*)data) + offset, length);
								memcpy(rightValue, ((char*)data) + offset - 4, length + 4);  //keep value has format (length)(varchar)
								offset += length;
							}else{
								//rightValue = malloc(4);
								memcpy(rightValue, ((char*)data) + offset, 4);
								offset += 4; //float or int
							}
        						//find, continue
        						attrType = attrs[i].type;
        						continue;
						}

        					//no match
        					if(attrs[i].type == TypeVarChar){
        						int length;
        						memcpy(&length, ((char*)data) + offset, sizeof(int));
        						offset += sizeof(int);
        						offset += length;
        					}else{
        						offset += 4;  //int or float
        					}
        				}

        				// after you iterate all the recordDescriptor,
        				// you get leftValue and rightValue, need to compare
        				if(this->compare(attrType, condition.op)){
//        					free(leftValue);
//        					free(rightValue);
        					return 0;
        				}
        			}else{
        				//assign rightValue
        				memcpy(rightValue, condition.rhsValue.data, sizeof(condition.rhsValue.data));
        				//rightValue = condition.rhsValue.data;

        				int offset = 0;
					int nullFieldsIndicatorActualSize = ceil((double) attrs.size() / CHAR_BIT);
					offset += nullFieldsIndicatorActualSize;

					AttrType attrType;

					// try to find leftValue
					for(int i = 0; i < attrs.size();i ++){
						//get leftValue
						if(attrs[i].name == condition.lhsAttr){
							if(attrs[i].type == TypeVarChar){
								int length;
								memcpy(&length, ((char*)data) + offset, sizeof(int));
								offset += sizeof(int);

								memcpy(leftValue, ((char*)data) + offset - 4, length);  //keep value has format (length)(varchar)
								offset += length;
							}else{
								memcpy(leftValue, ((char*)data) + offset, 4);
								offset += 4; //float or int
							}
							//find, continue
							attrType = attrs[i].type;

							// you get leftValue and rightValue, need to compare
							if(this->compare(attrType, condition.op)){
								return 0;
							}

							continue;
						}

						//no match
						if(attrs[i].type == TypeVarChar){
							int length;
							memcpy(&length, ((char*)data) + offset, sizeof(int));
							offset += sizeof(int);
							offset += length;
						}else{
							offset += 4;  //int or float
						}
					}
        			}

        		}
        		return QE_EOF;
        };
        // For attribute in vector<Attribute>, name it as rel.attr
        void getAttributes(vector<Attribute> &attrs) const{
        		attrs.clear();
        		input->getAttributes(attrs);
        };
};


class Project : public Iterator {
	Iterator *input;
	vector<Attribute> allAttrs;
	vector<Attribute> needAttrs;
	vector<string> needAttrNames;
	void* recordData;

    // Projection operator
    public:
        Project(Iterator *input,                    // Iterator of input R
              const vector<string> &attrNames);   // vector containing attribute names
        ~Project(){
        		free(recordData);
        };

        RC getNextTuple(void *data) {
        		while(input->getNextTuple(recordData) != QE_EOF){

        			/*
        			 * 1. null indicator
        			 * 2. check attr_name, if equal, then add to data
        			 * */

        			//using two pointer
        			int dataOffset = 0, recordOffset = 0;

        			int record_nullFieldsIndicatorActualSize = ceil((double) allAttrs.size() / CHAR_BIT);
        			int data_nullFieldsIndicatorActualSize = ceil((double) needAttrs.size() / CHAR_BIT);
        			//set null indicator
        			memset(((char*)data) + dataOffset, 0, data_nullFieldsIndicatorActualSize);

        			dataOffset += data_nullFieldsIndicatorActualSize;
        			recordOffset += record_nullFieldsIndicatorActualSize;

        			for(int i= 0; i < allAttrs.size(); i++){
        				bool isFind = false;
        				for(int j = 0; j < needAttrs.size(); j++){
        					//find attribute, append to data
        					if(allAttrs[i].name == needAttrs[j].name){
        						Attribute attr = needAttrs[j];

        						if(attr.type == TypeVarChar){
        							int length;
        							memcpy(&length, ((char*)recordData) + recordOffset, sizeof(int));

        							// copy (VarChar data with length) in one time
        							memcpy(((char*)data) + dataOffset, ((char*)recordData) + recordOffset, length + sizeof(int));
        							dataOffset += (length + sizeof(int));
        							recordOffset += (length + sizeof(int));
        						}else{
        							memcpy(((char*)data) + dataOffset, ((char*)recordData) + recordOffset, 4);
        							dataOffset += 4;
        							recordOffset += 4;
        						}

        						isFind = true;
        						//found in needAttr, check next attr from "allAttrs"
        						break;
        					}
        				}
        				//if not find attribute, recordOffset need to add
        				if(!isFind){
        					if(allAttrs[i].type == TypeVarChar){
							int length;
							memcpy(&length, ((char*)recordData) + recordOffset, sizeof(int));
							recordOffset += (sizeof(int) + length);
						}else{
							recordOffset += 4;
						}
        				}
        			}
        			return 0;
        		}
        		return QE_EOF;
        };

        // For attribute in vector<Attribute>, name it as rel.attr
        void getAttributes(vector<Attribute> &attrs) const{
        		attrs.clear();
        		for(int i = 0; i < allAttrs.size(); i++){
        			for(int j = 0; j < needAttrNames.size(); j++){
        				if(allAttrs[i].name == needAttrNames[j]){
        					attrs.push_back(allAttrs[i]);
        				}
        			}
        		}
        };
};

class BNLJoin : public Iterator {
    // Block nested-loop join operator

	Iterator *leftIn;
	TableScan *rightIn;
	Condition condition;
	unsigned numPages;
	vector<Attribute> leftAttrs;
	vector<Attribute> rightAttrs;
	void* leftValue;
	void* leftRecord;
	void* rightValue;
	void* rightRecord;
	bool isAfterFirstTimeLeft;

    public:
        BNLJoin(Iterator *leftIn,            // Iterator of input R
               TableScan *rightIn,           // TableScan Iterator of input S
               const Condition &condition,   // Join condition
               const unsigned numPages       // # of pages that can be loaded into memory,
			                                 //   i.e., memory block size (decided by the optimizer)
        );
        ~BNLJoin(){
        		free(leftValue);
        		free(leftRecord);
        		free(rightValue);
        		free(rightRecord);
        };

        bool compare(AttrType type, CompOp op){
			if(type == TypeInt){
				int leftValue_int;
				int rightValue_int;
				memcpy(&leftValue_int, leftValue, sizeof(int));
				memcpy(&rightValue_int, rightValue, sizeof(int));

//        			cout << "left:" << leftValue_int << "  right: " << rightValue_int << endl;
				// compare
				if(op == EQ_OP)			return (leftValue_int == rightValue_int);
				else if(op == LT_OP)		return (leftValue_int < rightValue_int);
				else if(op == LE_OP)		return (leftValue_int <= rightValue_int);
				else if(op == GT_OP)		return (leftValue_int > rightValue_int);
				else if(op == GE_OP)		return (leftValue_int >= rightValue_int);
				else if(op == NE_OP)		return (leftValue_int != rightValue_int);
			}else if(type == TypeReal){
				float leftValue_float;
				float rightValue_float;
				memcpy(&leftValue_float, leftValue, sizeof(float));
				memcpy(&rightValue_float, rightValue, sizeof(float));

				// compare
				if(op == EQ_OP)			return (leftValue_float == rightValue_float);
				else if(op == LT_OP)		return (leftValue_float < rightValue_float);
				else if(op == LE_OP)		return (leftValue_float <= rightValue_float);
				else if(op == GT_OP)		return (leftValue_float > rightValue_float);
				else if(op == GE_OP)		return (leftValue_float >= rightValue_float);
				else if(op == NE_OP)		return (leftValue_float != rightValue_float);
			}else if(type == TypeVarChar){
				int leftValue_length;
				int rightValue_length;
				memcpy(&leftValue_length, leftValue, sizeof(int));
				memcpy(&rightValue_length, rightValue, sizeof(int));

				void* leftValue_string = malloc(leftValue_length);
				void* rightValue_string = malloc(rightValue_length);
				memcpy(leftValue_string, ((char*)leftValue) + sizeof(int), leftValue_length);
				memcpy(rightValue_string, ((char*)rightValue) + sizeof(int), rightValue_length);

//				cout << "leftValue_length:" << leftValue_length << endl;
//				cout << "rightValue_length:" << rightValue_length << endl;
//				cout << "leftValue_string:" << *((char*)leftValue_string) << endl;
//				cout << "rightValue_string:" << *((char*)rightValue_string) << endl;

				// compare
				if(op == EQ_OP)			return (*((char*)leftValue_string) == *((char*)rightValue_string));
				else if(op == LT_OP)		return (*((char*)leftValue_string) < *((char*)rightValue_string));
				else if(op == LE_OP)		return (*((char*)leftValue_string) <= *((char*)rightValue_string));
				else if(op == GT_OP)		return (*((char*)leftValue_string) > *((char*)rightValue_string));
				else if(op == GE_OP)		return (*((char*)leftValue_string) >= *((char*)rightValue_string));
				else if(op == NE_OP)		return (*((char*)leftValue_string) != *((char*)rightValue_string));
				free(leftValue_string);
				free(rightValue_string);
			}

			return -99;
		};
        RC getNextTuple(void *data){
        		int rightOffset = 0, leftOffset = 0;

        		/*
        		 * use this to avoid leftRecord to be flushed
        		 *
        		 *
        		 * */
        		if(isAfterFirstTimeLeft){
        			goto AfterFirstTimeLeft;
        		}

        		//left loop
        		while(leftIn->getNextTuple(leftRecord) != QE_EOF){

        			//setup all pointer
        			leftOffset = 0;
        			rightIn->setIterator();
        			isAfterFirstTimeLeft = true;

        			/////
        			leftOffset += ceil((double) leftAttrs.size() / CHAR_BIT);

        			for(int i = 0; i < leftAttrs.size();i++){
        				//find leftValue
        				if(condition.lhsAttr == leftAttrs[i].name){
        					if(leftAttrs[i].type == TypeVarChar){
        						int length;
        						memcpy(&length, ((char*)leftRecord) + leftOffset, sizeof(int));
        						leftOffset += sizeof(int);

        						memcpy(leftValue, ((char*)leftRecord) + leftOffset - 4, length + 4);  // copy all data including length into value
        						leftOffset += length;
        					}else{
        						//int or float
        						memcpy(leftValue, ((char*)leftRecord) + leftOffset, 4);
        						leftOffset += 4;
        					}
        					break;
        				}
        				//no match, move offset
        				if(leftAttrs[i].type == TypeVarChar){
        					int length;
        					memcpy(&length, ((char*)leftRecord) + leftOffset, sizeof(int));
        					leftOffset += sizeof(int);
        					leftOffset += length;
        				}else{
        					leftOffset += 4;
        				}
        			}

			AfterFirstTimeLeft:

        			//right loop
        			AttrType attrType;
        			while(rightIn->getNextTuple(rightRecord) != QE_EOF){
        				rightOffset = 0;

        				rightOffset += ceil((double) rightAttrs.size() / CHAR_BIT);

        				for(int i = 0; i < rightAttrs.size(); i++){
        					//find rightValue
        					if(rightAttrs[i].name == condition.rhsAttr){
        						attrType = rightAttrs[i].type;

        						if(rightAttrs[i].type == TypeVarChar){
        							int length;
        							memcpy(&length, ((char*)rightRecord) + rightOffset, sizeof(int));
        							rightOffset += sizeof(int);

        							memcpy(rightValue, ((char*)rightRecord) + rightOffset - 4, length + 4); // (int)(varchar)
        							rightOffset += sizeof(length);
        						}else{
        							//int or float
        							memcpy(rightValue, ((char*)rightRecord) + rightOffset, 4);
        							rightOffset += 4;
        						}
        						break;
        					}
        					//no match, add offset
        					if(rightAttrs[i].type == TypeVarChar){
        						int length;
        						memcpy(&length, ((char*)rightRecord) + rightOffset, sizeof(int));
        						rightOffset += sizeof(int);
        						rightOffset += length;
        					}else{
        						rightOffset += 4;
        					}
        				}

        				//after get(or not get) rightValue, compare!
        				//compare value, if true, start to build data(returnData)
					if(compare(attrType, condition.op)){
						leftOffset = 0;
						rightOffset = 0;
						int dataOffset = 0; //only for data

						dataOffset += ceil((double) (leftAttrs.size() + rightAttrs.size()) / CHAR_BIT);
						leftOffset += ceil((double) leftAttrs.size() / CHAR_BIT);

						for(int i= 0; i < leftAttrs.size(); i++){
							if(leftAttrs[i].type == TypeVarChar){
								int length;
								memcpy(&length, ((char*)leftRecord) + leftOffset, sizeof(int));
								leftOffset += sizeof(int);
								dataOffset += sizeof(int);

								memcpy(((char*)data) + dataOffset - 4, ((char*)leftRecord) + leftOffset - 4, length + sizeof(int)); //(int)(varchar)
								leftOffset += length;
								dataOffset += length;
							}else{
								memcpy(((char*)data) + dataOffset, ((char*)leftRecord) + leftOffset, sizeof(int));
								leftOffset += 4;
								dataOffset += 4;
							}
						}

						rightOffset += ceil((double) rightAttrs.size() / CHAR_BIT);
						for(int i = 0; i < rightAttrs.size(); i++){
							if(rightAttrs[i].type == TypeVarChar){
								int length;
								memcpy(&length, ((char*)rightRecord) + rightOffset, sizeof(int));
								rightOffset += 4;
								dataOffset += 4;

								// data:  (leftRecord)(rightRecrod)
								memcpy(((char*)data) + dataOffset - 4,
										((char*)rightRecord) + rightOffset - 4,
										length + sizeof(int));
								rightOffset += length;
							}else{
								memcpy(((char*)data) + dataOffset, ((char*)rightRecord) + rightOffset, 4);
								rightOffset += 4;
								dataOffset += 4;
							}
						}

						return 0;
					}
        			}
        		}

        		return QE_EOF;
        };
        // For attribute in vector<Attribute>, name it as rel.attr
        void getAttributes(vector<Attribute> &attrs) const{
        		attrs.clear();
        		for(int i = 0; i < leftAttrs.size(); i++){
        			attrs.push_back(leftAttrs[i]);
        		}
        		for(int i = 0; i < rightAttrs.size(); i++){
        			attrs.push_back(rightAttrs[i]);
        		}
        };
};


class INLJoin : public Iterator {
    // Index nested-loop join operator
    public:

		Iterator *leftIn;
		IndexScan *rightIn;
		Condition condition;
		vector<Attribute> leftAttrs;
		vector<Attribute> rightAttrs;
		void* leftRecord;
		void* rightRecord;
		void* leftValue;
		void* rightValue;
		bool isAfterFirstTimeLeft;

        INLJoin(Iterator *leftIn,           // Iterator of input R
               IndexScan *rightIn,          // IndexScan Iterator of input S
               const Condition &condition   // Join condition
        );
        ~INLJoin(){
        		free(leftRecord);
        		free(rightRecord);
        		free(leftValue);
        		free(rightValue);
        };

        bool compare(AttrType type, CompOp op){
			if(type == TypeInt){
				int leftValue_int;
				int rightValue_int;
				memcpy(&leftValue_int, leftValue, sizeof(int));
				memcpy(&rightValue_int, rightValue, sizeof(int));

//        			cout << "left:" << leftValue_int << "  right: " << rightValue_int << endl;
				// compare
				if(op == EQ_OP)			return (leftValue_int == rightValue_int);
				else if(op == LT_OP)		return (leftValue_int < rightValue_int);
				else if(op == LE_OP)		return (leftValue_int <= rightValue_int);
				else if(op == GT_OP)		return (leftValue_int > rightValue_int);
				else if(op == GE_OP)		return (leftValue_int >= rightValue_int);
				else if(op == NE_OP)		return (leftValue_int != rightValue_int);
			}else if(type == TypeReal){
				float leftValue_float;
				float rightValue_float;
				memcpy(&leftValue_float, leftValue, sizeof(float));
				memcpy(&rightValue_float, rightValue, sizeof(float));

				// compare
				if(op == EQ_OP)			return (leftValue_float == rightValue_float);
				else if(op == LT_OP)		return (leftValue_float < rightValue_float);
				else if(op == LE_OP)		return (leftValue_float <= rightValue_float);
				else if(op == GT_OP)		return (leftValue_float > rightValue_float);
				else if(op == GE_OP)		return (leftValue_float >= rightValue_float);
				else if(op == NE_OP)		return (leftValue_float != rightValue_float);
			}else if(type == TypeVarChar){
				int leftValue_length;
				int rightValue_length;
				memcpy(&leftValue_length, leftValue, sizeof(int));
				memcpy(&rightValue_length, rightValue, sizeof(int));

				void* leftValue_string = malloc(leftValue_length);
				void* rightValue_string = malloc(rightValue_length);
				memcpy(leftValue_string, ((char*)leftValue) + sizeof(int), leftValue_length);
				memcpy(rightValue_string, ((char*)rightValue) + sizeof(int), rightValue_length);

//				cout << "leftValue_length:" << leftValue_length << endl;
//				cout << "rightValue_length:" << rightValue_length << endl;
//				cout << "leftValue_string:" << *((char*)leftValue_string) << endl;
//				cout << "rightValue_string:" << *((char*)rightValue_string) << endl;

				// compare
				if(op == EQ_OP)			return (*((char*)leftValue_string) == *((char*)rightValue_string));
				else if(op == LT_OP)		return (*((char*)leftValue_string) < *((char*)rightValue_string));
				else if(op == LE_OP)		return (*((char*)leftValue_string) <= *((char*)rightValue_string));
				else if(op == GT_OP)		return (*((char*)leftValue_string) > *((char*)rightValue_string));
				else if(op == GE_OP)		return (*((char*)leftValue_string) >= *((char*)rightValue_string));
				else if(op == NE_OP)		return (*((char*)leftValue_string) != *((char*)rightValue_string));
				free(leftValue_string);
				free(rightValue_string);
			}

			return -99;
		};

        RC getNextTuple(void *data){
        		int rightOffset = 0, leftOffset = 0;

//        		//use this to avoid leftRecord to be flushed
//			if(isAfterFirstTimeLeft){
//				goto AfterFirstTimeLeft2;
//			}
//
//			for(int i = 0; i < leftAttrs.size(); i++){
//				cout << leftAttrs[i].name << endl;
//			}
//
//			for(int i = 0; i < rightAttrs.size(); i++){
//				cout << rightAttrs[i].name << endl;
//			}
//

			//left loop
			while(leftIn->getNextTuple(leftRecord) != QE_EOF){

				//setup all pointer
				leftOffset = 0;
				rightIn->setIterator(NULL, NULL, true, true);
				isAfterFirstTimeLeft = true;

				/////
				leftOffset += ceil((double) leftAttrs.size() / CHAR_BIT);

				//get leftValue
				for(int i = 0; i < leftAttrs.size();i++){
					//find leftValue
					if(condition.lhsAttr == leftAttrs[i].name){
						if(leftAttrs[i].type == TypeVarChar){
							int length;
							memcpy(&length, ((char*)leftRecord) + leftOffset, sizeof(int));
							leftOffset += sizeof(int);

							memcpy(leftValue, ((char*)leftRecord) + leftOffset - 4, length + 4);  // copy all data including length into value
							leftOffset += length;
						}else{
							//int or float
							memcpy(leftValue, ((char*)leftRecord) + leftOffset, 4);
							leftOffset += 4;
						}
						break;
					}
					//no match, move offset
					if(leftAttrs[i].type == TypeVarChar){
						int length;
						memcpy(&length, ((char*)leftRecord) + leftOffset, sizeof(int));
						leftOffset += sizeof(int);
						leftOffset += length;
					}else{
						leftOffset += 4;
					}
				}
		AfterFirstTimeLeft2:

				//right loop
				AttrType attrType;
				while(rightIn->getNextTuple(rightRecord) != QE_EOF){
					rightOffset = 0;

					rightOffset += ceil((double) rightAttrs.size() / CHAR_BIT);

					for(int i = 0; i < rightAttrs.size(); i++){
						//find rightValue
						if(rightAttrs[i].name == condition.rhsAttr){
							attrType = rightAttrs[i].type;

							if(rightAttrs[i].type == TypeVarChar){
								int length;
								memcpy(&length, ((char*)rightRecord) + rightOffset, sizeof(int));
								rightOffset += sizeof(int);

								memcpy(rightValue, ((char*)rightRecord) + rightOffset - 4, length + 4); // (int)(varchar)
								rightOffset += sizeof(length);
							}else{
								//int or float
								memcpy(rightValue, ((char*)rightRecord) + rightOffset, 4);
								rightOffset += 4;
							}
							break;
						}
						//no match, add offset
						if(rightAttrs[i].type == TypeVarChar){
							int length;
							memcpy(&length, ((char*)rightRecord) + rightOffset, sizeof(int));
							rightOffset += sizeof(int);
							rightOffset += length;
						}else{
							rightOffset += 4;
						}
					}

					//after get(or not get) rightValue, compare!
					//compare value, if true, start to build data(returnData)
					if(compare(attrType, condition.op)){
						leftOffset = 0;
						rightOffset = 0;
						int dataOffset = 0; //only for data

						dataOffset += ceil((double) (leftAttrs.size() + rightAttrs.size()) / CHAR_BIT);
						leftOffset += ceil((double) leftAttrs.size() / CHAR_BIT);

						for(int i= 0; i < leftAttrs.size(); i++){
							if(leftAttrs[i].type == TypeVarChar){
								int length;
								memcpy(&length, ((char*)leftRecord) + leftOffset, sizeof(int));
								leftOffset += sizeof(int);
								dataOffset += sizeof(int);

								memcpy(((char*)data) + dataOffset - 4, ((char*)leftRecord) + leftOffset - 4, length + sizeof(int)); //(int)(varchar)
								leftOffset += length;
								dataOffset += length;
							}else{
								memcpy(((char*)data) + dataOffset, ((char*)leftRecord) + leftOffset, sizeof(int));
								leftOffset += 4;
								dataOffset += 4;
							}
						}

						rightOffset += ceil((double) rightAttrs.size() / CHAR_BIT);
						for(int i = 0; i < rightAttrs.size(); i++){
							if(rightAttrs[i].type == TypeVarChar){
								int length;
								memcpy(&length, ((char*)rightRecord) + rightOffset, sizeof(int));
								rightOffset += 4;
								dataOffset += 4;

								// data:  (leftRecord)(rightRecrod)
								memcpy(((char*)data) + dataOffset - 4,
										((char*)rightRecord) + rightOffset - 4,
										length + sizeof(int));
								rightOffset += length;
							}else{
								memcpy(((char*)data) + dataOffset, ((char*)rightRecord) + rightOffset, 4);
								rightOffset += 4;
								dataOffset += 4;
							}
						}
//						isAfterFirstTimeLeft = false;
						return 0;
					}
				}
				isAfterFirstTimeLeft = false;
			}
			return QE_EOF;
        };
        // For attribute in vector<Attribute>, name it as rel.attr
        void getAttributes(vector<Attribute> &attrs) const{
        		attrs.clear();
        		for(int i = 0; i < leftAttrs.size(); i++){
        			attrs.push_back(leftAttrs[i]);
        		}
        		for(int i = 0; i < rightAttrs.size(); i++){
        			attrs.push_back(rightAttrs[i]);
        		}
        };
};

// Optional for everyone. 10 extra-credit points
class GHJoin : public Iterator {
    // Grace hash join operator
    public:
      GHJoin(Iterator *leftIn,               // Iterator of input R
            Iterator *rightIn,               // Iterator of input S
            const Condition &condition,      // Join condition (CompOp is always EQ)
            const unsigned numPartitions     // # of partitions for each relation (decided by the optimizer)
      ){};
      ~GHJoin(){};

      RC getNextTuple(void *data){return QE_EOF;};
      // For attribute in vector<Attribute>, name it as rel.attr
      void getAttributes(vector<Attribute> &attrs) const{};
};

class Aggregate : public Iterator {
    // Aggregation operator
    public:
        // Mandatory
        // Basic aggregation
        Aggregate(Iterator *input,          // Iterator of input R
                  Attribute aggAttr,        // The attribute over which we are computing an aggregate
                  AggregateOp op            // Aggregate operation
        ){};

        // Optional for everyone: 5 extra-credit points
        // Group-based hash aggregation
        Aggregate(Iterator *input,             // Iterator of input R
                  Attribute aggAttr,           // The attribute over which we are computing an aggregate
                  Attribute groupAttr,         // The attribute over which we are grouping the tuples
                  AggregateOp op              // Aggregate operation
        ){};
        ~Aggregate(){};

        RC getNextTuple(void *data){return QE_EOF;};
        // Please name the output attribute as aggregateOp(aggAttr)
        // E.g. Relation=rel, attribute=attr, aggregateOp=MAX
        // output attrname = "MAX(rel.attr)"
        void getAttributes(vector<Attribute> &attrs) const{};
};

#endif
