#ifndef _rbfm_h_
#define _rbfm_h_

#include <string>
#include <vector>
#include <climits>

#include "../rbf/pfm.h"
#include <sys/stat.h>
#include <math.h>
using namespace std;

// Record ID
typedef struct {
	unsigned pageNum;    // page number
	unsigned slotNum;    // slot number in the page
} RID;

// Attribute
typedef enum {
	TypeInt = 0, TypeReal, TypeVarChar
} AttrType;

typedef unsigned AttrLength;

struct Attribute {
	string name;     // attribute name
	AttrType type;     // attribute type
	AttrLength length; // attribute length
};

// Comparison Operator (NOT needed for part 1 of the project)
typedef enum {
	EQ_OP = 0, // no condition// =
	LT_OP,      // <
	LE_OP,      // <=
	GT_OP,      // >
	GE_OP,      // >=
	NE_OP,      // !=
	NO_OP       // no condition
} CompOp;

/********************************************************************************
 The scan iterator is NOT required to be implemented for the part 1 of the project
 ********************************************************************************/

# define RBFM_EOF (-1)  // end of a scan operator

// RBFM_ScanIterator is an iterator to go through records
// The way to use it is like the following:
//  RBFM_ScanIterator rbfmScanIterator;
//  rbfm.open(..., rbfmScanIterator);
//  while (rbfmScanIterator(rid, data) != RBFM_EOF) {
//    process the data;
//  }
//  rbfmScanIterator.close();

class RBFM_ScanIterator {
public:
	RBFM_ScanIterator() {};
	~RBFM_ScanIterator() {};

	RID rid;
	int count = 0;
	bool isSetFirst = false;
	string tableName;
	string conditionAttribute;
	CompOp compOp = NO_OP;
	void *value = NULL;
	vector<string> attributeNames;
	FileHandle fileHandle;
	vector<Attribute> recordDescriptor;

	// Never keep the results in the memory. When getNextRecord() is called,
	// a satisfying record needs to be fetched from the file.
	// "data" follows the same format as RecordBasedFileManager::insertRecord().
	RC getNextRID(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor,
			const string &conditionAttribute, const CompOp compOp, // comparision type such as "<" and "="
			const void *value,                    // used in the comparison
			const vector<string> &attributeNames, // a list of projected attributes
			RBFM_ScanIterator &rbfm_ScanIterator);

	RC getNextRecord(RID &rid0, void *data) {
		RBFM_ScanIterator rbfm_scanIterator;
		if (!isSetFirst) {
			rbfm_scanIterator.rid.pageNum = 0;
			rbfm_scanIterator.rid.slotNum = 0;
			isSetFirst = true;
		} else {
			rbfm_scanIterator.rid = rid0;
		}

//		cout << "here" << endl;
		rbfm_scanIterator.getNextRID(fileHandle, recordDescriptor, conditionAttribute, compOp, ((char*) value), attributeNames, rbfm_scanIterator);
//		cout << "here2" << endl;

//		cout << "rbfm_scanIterator page:" << rbfm_scanIterator.rid.pageNum << endl;
//		cout << "rbfm_scanIterator slot:" << rbfm_scanIterator.rid.slotNum << endl;

		//if no match found
		if (rbfm_scanIterator.rid.slotNum == 4096) {
			return -1;
		}

		//get all the attribute index that need to be used as output
		int k = 0;
		vector<int> attrOutput;
		vector<AttrType> attrOutputType;

		//cout << "recordDescriptor.size():" << recordDescriptor.size() << endl;

		for (int i = 0; i < recordDescriptor.size(); i++) {
			if (k<attributeNames.size() && recordDescriptor[i].name == attributeNames[k]) {
				k++;
				attrOutput.push_back(i);
				attrOutputType.push_back(recordDescriptor[i].type);
			}
		}
		//read the record
		void* pageData = malloc(PAGE_SIZE);
		RC rc = fileHandle.readPage(rbfm_scanIterator.rid.pageNum, pageData);
		if(rc != 0)
			return -1;

		short NF[2];
		memcpy(NF, ((char*) pageData) + 4092, sizeof(short) * 2);


		short slotInfo[2];
		memcpy(slotInfo, ((char*) pageData) + 4096 - 4 - ((rbfm_scanIterator.rid.slotNum+1) * sizeof(short) * 2), sizeof(short) * 2);

		short addr;
		//										NF
		memcpy(&addr, ((char*) pageData) + 4096 - 4 - ((rbfm_scanIterator.rid.slotNum+1) * sizeof(short) * 2), sizeof(short));

		int fieldCount;
		memcpy(&fieldCount, ((char*) pageData) + addr + 1, sizeof(int));
		//cout << endl;

		int offset = 0;
		int nullByte = ceil((double) attrOutput.size() / CHAR_BIT);
		void* nullIndicator = malloc(nullByte);
		memset(nullIndicator, 0, nullByte);
		memcpy(((char *) data) + offset, nullIndicator, nullByte);
		offset += nullByte;
		free(nullIndicator);

		for (int i = 0; i < attrOutput.size(); i++) {
			short pos = 0;
			if (attrOutput[i] == 0) { //the first attribute
				pos = 1 + sizeof(int) + (fieldCount * sizeof(short)) + nullByte; //indicator + all slots + nullindicator
			} else {
				// move to recordStart + indicator(1byte) + fieldCount(4bytes) + offsets
				memcpy(&pos, ((char*) pageData) + slotInfo[0] + sizeof(char) + sizeof(int) + (attrOutput[i] - 1) * sizeof(short), sizeof(short));
			}

			if (attrOutputType[i] == 0) { //int
				int test;
				memcpy(&test, ((char*) pageData) + addr + pos, sizeof(float));
				memcpy(((char *) data) + offset, ((char*) pageData) + addr + pos, sizeof(int));
				offset += sizeof(int);
			}
			if (attrOutputType[i] == 1) { //real
				float test;
				memcpy(&test, ((char*) pageData) + addr + pos, sizeof(float));
				memcpy(((char *) data) + offset, ((char*) pageData) + addr + pos, sizeof(float));
				offset += sizeof(float);

			}
			if (attrOutputType[i] == 2) { //char
				int length;
				memcpy(&length, ((char*) pageData) + addr + pos, sizeof(int));

				memcpy(((char *) data) + offset, &length, sizeof(int));
				offset += sizeof(int);

				memcpy(((char *) data) + offset, ((char*) pageData) + addr + pos + 4, length);
				offset += length;
			}
		}
		free(pageData);

		// slot number +1
		// if slot number is bigger than N turn to the next page
		// if is already at the last page turn slot number to 4096

		//has problems
		int page = fileHandle.getNumberOfPages();

		short slotsNums;
		memcpy(&slotsNums, ((char*) pageData) + 4096 - 4, sizeof(short));

		if (rbfm_scanIterator.rid.pageNum == page && rbfm_scanIterator.rid.slotNum == slotsNums - 1) {
			rbfm_scanIterator.rid.slotNum = 4096;
			rid0 = rbfm_scanIterator.rid;
			return -1;
		} else if (rbfm_scanIterator.rid.slotNum == slotsNums - 1) {
			rbfm_scanIterator.rid.slotNum = 0;
			rbfm_scanIterator.rid.pageNum++;
			rid0 = rbfm_scanIterator.rid;
			return 0;
		} else {
			rbfm_scanIterator.rid.slotNum++;
			rid0 = rbfm_scanIterator.rid;
			return 0;
		}
	};
	RC close() {
		return -1;
	};
};

class RecordBasedFileManager {
public:
	static RecordBasedFileManager* instance();

	RC createFile(const string &fileName);

	RC destroyFile(const string &fileName);

	RC openFile(const string &fileName, FileHandle &fileHandle);

	RC closeFile(FileHandle &fileHandle);

	//  Format of the data passed into the function is the following:
	//  [n byte-null-indicators for y fields] [actual value for the first field] [actual value for the second field] ...
	//  1) For y fields, there is n-byte-null-indicators in the beginning of each record.
	//     The value n can be calculated as: ceil(y / 8). (e.g., 5 fields => ceil(5 / 8) = 1. 12 fields => ceil(12 / 8) = 2.)
	//     Each bit represents whether each field value is null or not.
	//     If k-th bit from the left is set to 1, k-th field value is null. We do not include anything in the actual data part.
	//     If k-th bit from the left is set to 0, k-th field contains non-null values.
	//     If there are more than 8 fields, then you need to find the corresponding byte first,
	//     then find a corresponding bit inside that byte.
	//  2) Actual data is a concatenation of values of the attributes.
	//  3) For Int and Real: use 4 bytes to store the value;
	//     For Varchar: use 4 bytes to store the length of characters, then store the actual characters.
	//  !!! The same format is used for updateRecord(), the returned data of readRecord(), and readAttribute().
	// For example, refer to the Q8 of Project 1 wiki page.
	RC insertRecord(FileHandle &fileHandle,
			const vector<Attribute> &recordDescriptor, const void *data,
			RID &rid);

	RC IXinsertRecord(FileHandle &fileHandle,
			const vector<Attribute> &recordDescriptor, const void *data,
			RID &rid, int page);

	RC readRecord(FileHandle &fileHandle,
			const vector<Attribute> &recordDescriptor, const RID &rid,
			void *data);

	// This method will be mainly used for debugging/testing.
	// The format is as follows:
	// field1-name: field1-value  field2-name: field2-value ... \n
	// (e.g., age: 24  height: 6.1  salary: 9000
	//        age: NULL  height: 7.5  salary: 7500)
	RC printRecord(const vector<Attribute> &recordDescriptor, const void *data);

	//my function
	short getNeededInsertRecordSize(const vector<Attribute> &recordDescriptor,
			const void *data); //get the size need to insert this record(not only the record, but also the record directory)
	int getPageHasSapce(int needSpace, FileHandle &fileHandle);	// check whether there's a page which has that size
	short getPageSlotNumber(int page, FileHandle &fileHandle);// get how many slots in this page
	short getSlotStartPosition(void* pageData, int pageSlots); // get the slot start position (last record end)

	RC insert(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor,
			const void *data, RID &rid);  // insert data

	/******************************************************************************************************************************************************************
	 IMPORTANT, PLEASE READ: All methods below this comment (other than the constructor and destructor) are NOT required to be implemented for the part 1 of the project
	 ******************************************************************************************************************************************************************/
	RC deleteRecord(FileHandle &fileHandle,
			const vector<Attribute> &recordDescriptor, const RID &rid);

	// Assume the RID does not change after an update
	RC updateRecord(FileHandle &fileHandle,
			const vector<Attribute> &recordDescriptor, const void *data,
			const RID &rid);

	RC readAttribute(FileHandle &fileHandle,
			const vector<Attribute> &recordDescriptor, const RID &rid,
			const string &attributeName, void *data);

	// Scan returns an iterator to allow the caller to go through the results one by one.
	RC scan(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor,
			const string &conditionAttribute,
			const CompOp compOp, // comparision type such as "<" and "="
			const void *value,                    // used in the comparison
			const vector<string> &attributeNames, // a list of projected attributes
			RBFM_ScanIterator &rbfm_ScanIterator);

public:

protected:
	RecordBasedFileManager();
	~RecordBasedFileManager();

private:
	static RecordBasedFileManager *_rbf_manager;
	//my function
	int findInsertSlotPosition(void* pageData, int &tag);
	void formatRecord(void* recordData, const void* data, const vector<Attribute> &recordDescriptor, short needSize);
};

#endif
