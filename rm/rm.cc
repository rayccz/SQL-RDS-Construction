
#include "rm.h"

RelationManager* RelationManager::instance()
{
    static RelationManager _rm;
    return &_rm;
}

RelationManager::RelationManager()
{

}

RelationManager::~RelationManager()
{

}

void createTablesDescriptor(vector<Attribute> &recordDescriptor) {
    Attribute attr;
    attr.name = "table-id";
    attr.type = TypeInt;
    attr.length = (AttrLength)4;
    recordDescriptor.push_back(attr);

    attr.name = "table-name";
    attr.type = TypeVarChar;
    attr.length = (AttrLength)50;
    recordDescriptor.push_back(attr);

    attr.name = "file-name";
    attr.type = TypeVarChar;
    attr.length = (AttrLength)50;
    recordDescriptor.push_back(attr);
}

void createColumnsDescriptor(vector<Attribute> &recordDescriptor) {
    Attribute attr;
    attr.name = "table-id";
    attr.type = TypeInt;
    attr.length = (AttrLength)4;
    recordDescriptor.push_back(attr);

    attr.name = "column-name";
    attr.type = TypeVarChar;
    attr.length = (AttrLength)50;
    recordDescriptor.push_back(attr);

    attr.name = "column-type";
    attr.type = TypeInt;
    attr.length = (AttrLength)4;
    recordDescriptor.push_back(attr);

    attr.name = "column-length";
	attr.type = TypeInt;
	attr.length = (AttrLength)4;
	recordDescriptor.push_back(attr);

	attr.name = "column-position";
	attr.type = TypeInt;
	attr.length = (AttrLength)4;
	recordDescriptor.push_back(attr);
}

void prepareTableRecord(int fieldCount, unsigned char *nullFieldsIndicator,
		const int tableid, const int tablenameLength, const string &tablename,
		const int filenameLength, const string &filename, void *buffer,
		int *recordSize) {
	int offset = 0;

	// Null-indicators
	bool nullBit = false;
	int nullFieldsIndicatorActualSize = ceil((double) fieldCount / CHAR_BIT);

	// Null-indicator for the fields
	memcpy((char *) buffer + offset, nullFieldsIndicator, nullFieldsIndicatorActualSize);
	offset += nullFieldsIndicatorActualSize;

	// Beginning of the actual data
	// Note that the left-most bit represents the first field. Thus, the offset is 7 from right, not 0.
	// e.g., if a record consists of four fields and they are all nulls, then the bit representation will be: [11110000]

	// int
	nullBit = nullFieldsIndicator[0] & (1 << 7);
	if (!nullBit) {
		memcpy((char *) buffer + offset, &tableid, sizeof(int));
		offset += sizeof(int);
	}

	// char
	nullBit = nullFieldsIndicator[0] & (1 << 6);
	if (!nullBit) {
		memcpy((char *) buffer + offset, &tablenameLength, sizeof(int));
		offset += sizeof(int);
		memcpy((char *) buffer + offset, tablename.c_str(), tablenameLength);
		offset += tablenameLength;
	}

	// char
	nullBit = nullFieldsIndicator[0] & (1 << 5);
	if (!nullBit) {
		memcpy((char *) buffer + offset, &filenameLength, sizeof(int));
		offset += sizeof(int);
		memcpy((char *) buffer + offset, filename.c_str(), filenameLength);
		offset += filenameLength;
	}

	*recordSize = offset;
}

void prepareColumnRecord(int fieldCount, unsigned char *nullFieldsIndicator,
		const int tableid, const int columnnameLength, const string &columnname,
		const int columntype, const int columnlength, const int columnposition,
		void *buffer, int *recordSize) {
	int offset = 0;

	// Null-indicators
	bool nullBit = false;
	int nullFieldsIndicatorActualSize = ceil((double) fieldCount / CHAR_BIT);

	// Null-indicator for the fields
	memcpy((char *) buffer + offset, nullFieldsIndicator, nullFieldsIndicatorActualSize);
	offset += nullFieldsIndicatorActualSize;

	// Beginning of the actual data
	// Note that the left-most bit represents the first field. Thus, the offset is 7 from right, not 0.
	// e.g., if a record consists of four fields and they are all nulls, then the bit representation will be: [11110000]

	// int
	nullBit = nullFieldsIndicator[0] & (1 << 7);
	if (!nullBit) {
		memcpy((char *) buffer + offset, &tableid, sizeof(int));
		offset += sizeof(int);
	}

	// char
	nullBit = nullFieldsIndicator[0] & (1 << 6);
	if (!nullBit) {
		memcpy((char *) buffer + offset, &columnnameLength, sizeof(int));
		offset += sizeof(int);
		memcpy((char *) buffer + offset, columnname.c_str(), columnnameLength);
		offset += columnnameLength;
	}

	// int
	nullBit = nullFieldsIndicator[0] & (1 << 5);
	if (!nullBit) {
		memcpy((char *) buffer + offset, &columntype, sizeof(int));
		offset += sizeof(int);
	}
	// int
	nullBit = nullFieldsIndicator[0] & (1 << 4);
	if (!nullBit) {
		memcpy((char *) buffer + offset, &columnlength, sizeof(int));
		offset += sizeof(int);
	}
	// int
	nullBit = nullFieldsIndicator[0] & (1 << 3);
	if (!nullBit) {
		memcpy((char *) buffer + offset, &columnposition, sizeof(int));
		offset += sizeof(int);
	}

	*recordSize = offset;
}

RC RelationManager::createCatalog()
{
	vector<Attribute> recordDescriptor1;
	vector<Attribute> recordDescriptor2;
	createTablesDescriptor(recordDescriptor1);
	createColumnsDescriptor (recordDescriptor2);

	int set =0;
	set=PagedFileManager::instance()->createFile("Tables");
	set=PagedFileManager::instance()->createFile("Columns");
	if (set==0){
		// Initialize a NULL field indicator
		int nullFieldsIndicatorActualSize1 = ceil((double) recordDescriptor1.size() / CHAR_BIT);
		unsigned char *nullsIndicator1 = (unsigned char *) malloc(nullFieldsIndicatorActualSize1);
		memset(nullsIndicator1, 0, nullFieldsIndicatorActualSize1);

		FileHandle fileHandleMaxID;
		PagedFileManager::instance()->createFile("maxtableid");
		RecordBasedFileManager::instance()->openFile("maxtableid", fileHandleMaxID);

		FileHandle fileHandle1;
		FileHandle fileHandle2;
		RID rid1;
		RID rid2;
		int recordSize1 = 0;
		void *record1 = malloc(150);
		prepareTableRecord(recordDescriptor1.size(), nullsIndicator1, 1, 6,"Tables",6,"Tables", record1, &recordSize1);
		RecordBasedFileManager::instance()->openFile("Tables", fileHandle1);

		RecordBasedFileManager::instance()->insertRecord(fileHandle1, recordDescriptor1, record1, rid1);
		this->maxtableid++;

		prepareTableRecord(recordDescriptor1.size(), nullsIndicator1, 2, 7,"Columns",7,"Columns", record1, &recordSize1);
		RecordBasedFileManager::instance()->insertRecord(fileHandle1, recordDescriptor1, record1, rid1);
		this->maxtableid++;

		fwrite (&(this->maxtableid) , sizeof(int), 1, fileHandleMaxID.file);
		RecordBasedFileManager::instance()->closeFile(fileHandleMaxID);

		RecordBasedFileManager::instance()->closeFile(fileHandle1);
		free(record1);

		int nullFieldsIndicatorActualSize2 = ceil((double) recordDescriptor1.size() / CHAR_BIT);
		unsigned char *nullsIndicator2 = (unsigned char *) malloc(nullFieldsIndicatorActualSize2);
		memset(nullsIndicator2, 0, nullFieldsIndicatorActualSize2);
		int recordSize2 = 0;
		void *record2 = malloc(150);
		RecordBasedFileManager::instance()->openFile("Columns", fileHandle2);

		prepareColumnRecord(recordDescriptor2.size(), nullsIndicator2, 1, 8,"table-id", TypeInt, 4, 1, record2, &recordSize2);
		RecordBasedFileManager::instance()->insertRecord(fileHandle2, recordDescriptor2, record2, rid2);

		prepareColumnRecord(recordDescriptor2.size(), nullsIndicator2, 1, 10,"table-name", TypeVarChar, 50, 2, record2, &recordSize2);
		RecordBasedFileManager::instance()->insertRecord(fileHandle2, recordDescriptor2, record2, rid2);

		prepareColumnRecord(recordDescriptor2.size(), nullsIndicator2, 1, 9,"file-name", TypeVarChar, 50, 3, record2, &recordSize2);
		RecordBasedFileManager::instance()->insertRecord(fileHandle2, recordDescriptor2, record2, rid2);

		prepareColumnRecord(recordDescriptor2.size(), nullsIndicator2, 2, 8,"table-id", TypeInt, 4, 1, record2, &recordSize2);
		RecordBasedFileManager::instance()->insertRecord(fileHandle2, recordDescriptor2, record2, rid2);

		prepareColumnRecord(recordDescriptor2.size(), nullsIndicator2, 2, 11,"column-name", TypeVarChar, 50, 2, record2, &recordSize2);
		RecordBasedFileManager::instance()->insertRecord(fileHandle2, recordDescriptor2, record2, rid2);

		prepareColumnRecord(recordDescriptor2.size(), nullsIndicator2, 2, 11,"column-type", TypeVarChar, 50, 3, record2, &recordSize2);
		RecordBasedFileManager::instance()->insertRecord(fileHandle2, recordDescriptor2, record2, rid2);

		prepareColumnRecord(recordDescriptor2.size(), nullsIndicator2, 2, 13,"column-length", TypeVarChar, 50, 4, record2, &recordSize2);
		RecordBasedFileManager::instance()->insertRecord(fileHandle2, recordDescriptor2, record2, rid2);

		prepareColumnRecord(recordDescriptor2.size(), nullsIndicator2, 2, 15,"column-position", TypeVarChar, 50, 5, record2, &recordSize2);
		RecordBasedFileManager::instance()->insertRecord(fileHandle2, recordDescriptor2, record2, rid2);

		RecordBasedFileManager::instance()->closeFile(fileHandle2);


		free(record2);
		free(nullsIndicator1);
		free(nullsIndicator2);
	}

	return set;
}

RC RelationManager::deleteCatalog()
{
	int set=0;
	set=PagedFileManager::instance()->destroyFile("Tables");
	set=PagedFileManager::instance()->destroyFile("Columns");
	PagedFileManager::instance()->destroyFile("maxtableid");
	return set;
}

RC RelationManager::createTable(const string &tableName, const vector<Attribute> &attrs)
{
	FileHandle fileHandle1;
	FileHandle fileHandle2;
	vector<Attribute> recordDescriptor1;
	vector<Attribute> recordDescriptor2;
	createTablesDescriptor(recordDescriptor1);
	createColumnsDescriptor(recordDescriptor2);
	RID rid1;
	RID rid2;

	// add the table into table of table
	int nullFieldsIndicatorActualSize1 = ceil((double) recordDescriptor1.size() / CHAR_BIT);
	unsigned char *nullsIndicator1 = (unsigned char *) malloc(nullFieldsIndicatorActualSize1);
	memset(nullsIndicator1, 0, nullFieldsIndicatorActualSize1);
	void *record1 = malloc(150);
	int recordSize1 = 0;

	// get table id
//	void *returnedData = malloc(150);

	FileHandle fileHandleMaxID;
	RecordBasedFileManager::instance()->openFile("maxtableid", fileHandleMaxID);
	fseek (fileHandleMaxID.file , 0 , SEEK_SET);
	fread (&(this->maxtableid),1,1,fileHandleMaxID.file);

	RecordBasedFileManager::instance()->openFile("Tables", fileHandle1);
	this->maxtableid++;

	fseek (fileHandleMaxID.file , 0 , SEEK_SET);
	fwrite (&(this->maxtableid) , sizeof(int), 1, fileHandleMaxID.file);
	RecordBasedFileManager::instance()->closeFile(fileHandleMaxID);

	prepareTableRecord(recordDescriptor1.size(), nullsIndicator1, this->maxtableid,
			tableName.length(), tableName, tableName.length(), tableName, record1, &recordSize1);

	RecordBasedFileManager::instance()->insertRecord(fileHandle1, recordDescriptor1, record1, rid1);
	RecordBasedFileManager::instance()->closeFile(fileHandle1);
	free(record1);

	// add the table attribute into the columns table
	int nullFieldsIndicatorActualSize2 = ceil((double) recordDescriptor2.size() / CHAR_BIT);
	unsigned char *nullsIndicator2 = (unsigned char *) malloc(nullFieldsIndicatorActualSize2);
	memset(nullsIndicator2, 0, nullFieldsIndicatorActualSize2);
	void *record2 = malloc(150);
	int recordSize2 = 0;

	RecordBasedFileManager::instance()->openFile("Columns", fileHandle2);
	for (int i = 0; i < attrs.size(); i++) {
		prepareColumnRecord(recordDescriptor1.size(), nullsIndicator2, this->maxtableid,
				attrs[i].name.length(), attrs[i].name, attrs[i].type, attrs[i].length, i + 1, record2, &recordSize2);
		RecordBasedFileManager::instance()->insertRecord(fileHandle2, recordDescriptor2, record2, rid2);
	}
	RecordBasedFileManager::instance()->closeFile(fileHandle2);
	free(record2);

	free(nullsIndicator1);
	free(nullsIndicator2);

	//create actual RBF file
	PagedFileManager::instance()->createFile(tableName);

	return 0;
}

RC RelationManager::deleteTable(const string &tableName)
{
	if (tableName=="Tables" || tableName=="Columns"){
		return -1;
	}
	int set=PagedFileManager::instance()->destroyFile(tableName);
    return set;
}

RC RelationManager::getAttributes(const string &tableName, vector<Attribute> &attrs)
{
	vector<Attribute> recordDescriptor1;
	vector<Attribute> recordDescriptor2;
	createTablesDescriptor(recordDescriptor1);
	createColumnsDescriptor(recordDescriptor2);

	RID tableRID;
	RID columnRID;
	FileHandle fileHandle1;
	FileHandle fileHandle2;
	int tableID;

	RecordBasedFileManager::instance()->openFile("Tables", fileHandle1);
	int page = fileHandle1.getNumberOfPages();

	//read pageData
	void* pageData = malloc(PAGE_SIZE);

	int check = 0;

	//find the table name from the table of table and return the table id
	for (int i = 0; i < page; i++) {
		fileHandle1.readPage(i, pageData);

		short slotsNums;
		memcpy(&slotsNums, ((char*) pageData) + 4096 - 4, sizeof(short));

		if (check == 1) {
			break;
		}

		for (int ii = 0; ii < slotsNums; ii++) {
			tableRID.pageNum = i;
			tableRID.slotNum = ii;

			short addr;
			memcpy(&addr, ((char*) pageData) + 4096 - 4 - 4 - ii * 2 * sizeof(short), sizeof(short));
			//get the length of table name

			char indicator;  // means this record not a RID
			memcpy(&indicator, ((char*) pageData) + addr, sizeof(char)); //indicator

			//char s = 0;
			if (indicator == 0) {
				int fildcount;
				memcpy(&fildcount, ((char*) pageData) + addr + 1, sizeof(int));

				short tidOffset;  // use tidOffset can move to the tid's end
				//	recordOffset + indicator(1byte) + fieldCount(4bytes)
				memcpy(&tidOffset, ((char*) pageData) + addr + 1 + sizeof(int), sizeof(short));

				int tableNameLength;
				memcpy(&tableNameLength, ((char*) pageData) + addr + tidOffset, sizeof(int));

				string tmp = "";
				char c;
				for (int cc = 0; cc < tableNameLength; cc++) {
					memcpy(&c, ((char*) pageData) + addr + tidOffset + 4 + cc, sizeof(char));
					tmp = tmp + c;
				}

				if (tmp == tableName) {
					memcpy(&tableID, ((char*) pageData) + addr + 12, sizeof(int));
					check = 1;
				}
			}
			if (check == 1) {
				break;
			}
		}
	}


	// find all the attributes
	RecordBasedFileManager::instance()->openFile("Columns", fileHandle2);
	int pagec = fileHandle2.getNumberOfPages();
	void* pageData2 = malloc(PAGE_SIZE);

	for (int i = 0; i < pagec; i++) {
		fileHandle2.readPage(i, pageData2);

		short slotsNums;
		memcpy(&slotsNums, ((char*) pageData2) + 4096 - 4, sizeof(short));

		for (int ii = 0; ii < slotsNums; ii++) {
			columnRID.pageNum = i;
			columnRID.slotNum = ii;

			short addr;
			memcpy(&addr, ((char*) pageData2) + 4096 - 4 - 4 - ii * 2 * sizeof(short), sizeof(short));
			//get the length of table name

			char indicator;  // means this record not a RID
			memcpy(&indicator, ((char*) pageData2) + addr, sizeof(char)); //indicator

			char s = 0;
			if (indicator == s) {
				int fildcount;
				memcpy(&fildcount, ((char*) pageData2) + addr + 1, sizeof(int));

				short tid;
				memcpy(&tid, ((char*) pageData2) + addr + 1 + 4, sizeof(short));

				short name;
				memcpy(&name, ((char*) pageData2) + addr + 1 + 4 + 2, sizeof(short));

				short type;
				memcpy(&type, ((char*) pageData2) + addr + 1 + 4 + 2 + 2, sizeof(short));

				short length;
				memcpy(&length, ((char*) pageData2) + addr + 1 + 4 + 2 + 2 + 2, sizeof(short));

				short position;
				memcpy(&position, ((char*) pageData2) + addr + 1 + 4 + 2 + 2 + 2 + 2, sizeof(short));

				int id;
				memcpy(&id, ((char*) pageData2) + addr + 1 + 4 + 2 + 2 + 2 + 2 + 2 + 1, sizeof(int));

				if (id == tableID) {

					int attrnamelen;
					memcpy(&attrnamelen, ((char*) pageData2) + addr + tid, sizeof(int));

					string attrname;

					char c;
					for (int cc = 0; cc < attrnamelen; cc++) {
						memcpy(&c, ((char*) pageData2) + addr + tid + 4 + cc, sizeof(char));
						attrname = attrname + c;
					}

					int attrtype;
					memcpy(&attrtype, ((char*) pageData2) + addr + name, sizeof(int));

					int attrlength;
					memcpy(&attrlength, ((char*) pageData2) + addr + type, sizeof(int));

					Attribute attr;
					attr.name = attrname;
					if (attrtype == 0) {
						attr.type = TypeInt;
					} else if (attrtype == 1) {
						attr.type = TypeReal;
					} else {
						attr.type = TypeVarChar;
					}
					attr.length = (AttrLength) attrlength;
					attrs.push_back(attr);

				}
			}
		}
	}
	RecordBasedFileManager::instance()->closeFile(fileHandle1);
	RecordBasedFileManager::instance()->closeFile(fileHandle2);

	free(pageData2);
	free(pageData);
	return 0;
}

RC RelationManager::insertTuple(const string &tableName, const void *data, RID &rid)
{
	FileHandle fileHandle;
	RC rc = 0;
	rc = RecordBasedFileManager::instance()->openFile(tableName, fileHandle);
	if(rc != 0)
		return - 1;
	vector<Attribute> recordDescriptor;
	rc = getAttributes(tableName, recordDescriptor);
	if(rc != 0)
		return -1;

	rc = RecordBasedFileManager::instance()->insertRecord(fileHandle, recordDescriptor, data, rid);
	rc = RecordBasedFileManager::instance()->closeFile(fileHandle);

	/*
	 1. also need to insertEntry()
	    how to do? check each attribute whether has the index_file
	*/

	for(int i = 0; i < recordDescriptor.size(); i++){
		//try to open index_file
		string indexFileName = tableName + "_" + recordDescriptor[i].name;
		IXFileHandle indexFileHandle;

		rc = IndexManager::instance()->openFile(indexFileName, indexFileHandle);

		//there's no this index_file, keep searching next
		if(rc == -1){
//			cout << "continue~" << endl;
			continue;
		}
//		cout << "not continue~" << endl;

		Attribute attr = recordDescriptor[i];
		void* attrData = malloc(PAGE_SIZE);

		readAttribute(tableName, rid, attr.name, attrData);
		rc = IndexManager::instance()->insertEntry(indexFileHandle, attr, data, rid);
		if(rc == RM_EOF){
			cout << "insert failed" << endl;
			return -1;
		}

		rc = IndexManager::instance()->closeFile(indexFileHandle);
		if(rc == RM_EOF){
			cout << "close fail" << endl;
			return -1;
		}
		free(attrData);
	}

    return 0;
}

RC RelationManager::deleteTuple(const string &tableName, const RID &rid)
{
	FileHandle fileHandle;
	RC rc = 0;
	rc = RecordBasedFileManager::instance()->openFile(tableName, fileHandle);
	if(rc != 0)
		return - 1;
	vector<Attribute> recordDescriptor;
	rc = getAttributes(tableName, recordDescriptor);
	if(rc != 0)
		return -1;

	rc = RecordBasedFileManager::instance()->deleteRecord(fileHandle, recordDescriptor, rid);
	if(rc != 0)
		return -1;

	rc = RecordBasedFileManager::instance()->closeFile(fileHandle);

    return 0;
}

RC RelationManager::updateTuple(const string &tableName, const void *data, const RID &rid)
{
	FileHandle fileHandle;
	RC rc = 0;
	rc = RecordBasedFileManager::instance()->openFile(tableName, fileHandle);
	if(rc != 0)
		return - 1;
	vector<Attribute> recordDescriptor;
	rc = getAttributes(tableName, recordDescriptor);
	if(rc != 0)
		return -1;

	rc = RecordBasedFileManager::instance()->updateRecord(fileHandle, recordDescriptor, data, rid);
	if(rc != 0)
		return -1;
	rc = RecordBasedFileManager::instance()->closeFile(fileHandle);

	return 0;
}

RC RelationManager::readTuple(const string &tableName, const RID &rid, void *data)
{
	FileHandle fileHandle;
	RC rc = 0;
	rc = RecordBasedFileManager::instance()->openFile(tableName, fileHandle);
	if(rc != 0)
		return - 1;
	vector<Attribute> recordDescriptor;
	rc = getAttributes(tableName, recordDescriptor);
	if(rc != 0)
		return -1;

	rc = RecordBasedFileManager::instance()->readRecord(fileHandle, recordDescriptor, rid, data);
	if(rc != 0)
		return -1;
	rc = RecordBasedFileManager::instance()->closeFile(fileHandle);

    return 0;
}

RC RelationManager::printTuple(const vector<Attribute> &attrs, const void *data)
{
	RC rc = 0;
	rc = RecordBasedFileManager::instance()->printRecord(attrs, data);
	return 0;
	//return -1;
}

RC RelationManager::readAttribute(const string &tableName, const RID &rid, const string &attributeName, void *data)
{
	RM_ScanIterator rmsi;
	vector<Attribute> recordDescriptor;
	getAttributes(tableName, recordDescriptor);

	vector<string> attributeNames;
	attributeNames.push_back(attributeName);
	this->scan(tableName, "", NO_OP, NULL, attributeNames, rmsi);

	RID tempRID;
	RID preRID;
	preRID.pageNum = 0;
	preRID.slotNum = 0;

	//getNextTuple always return nextRID
	while(rmsi.getNextTuple(tempRID, data) != RM_EOF){
		if(preRID.pageNum == rid.pageNum && preRID.slotNum == rid.slotNum){
			break;
		}
		preRID = tempRID;
	}

    return 0;
}

RC RelationManager::scan(const string &tableName,
      const string &conditionAttribute,
      const CompOp compOp,                  
      const void *value,                    
      const vector<string> &attributeNames,
      RM_ScanIterator &rm_ScanIterator)
{
	FileHandle fileHandle;
	RC rc = 0;
	rc = RecordBasedFileManager::instance()->openFile(tableName, fileHandle);
	if(rc != 0)
		return - 1;
	vector<Attribute> recordDescriptor;
	rc = getAttributes(tableName, recordDescriptor);
	if(rc != 0)
		return -1;
	rc = RecordBasedFileManager::instance()->scan(fileHandle, recordDescriptor, conditionAttribute, compOp,
												value, attributeNames, rm_ScanIterator.rbfm_ScanIterator);
	if(rc != 0)
		return -1;

    return 0;
}

// Extra credit work
RC RelationManager::dropAttribute(const string &tableName, const string &attributeName)
{
    return -1;
}

// Extra credit work
RC RelationManager::addAttribute(const string &tableName, const Attribute &attr)
{
    return -1;
}


RC RelationManager::createIndex(const string &tableName, const string &attributeName)
{
	// 1. create index file (if there's no file)
	// 2. find index attribute info (also check whether the attribute exist or not)
	// 3. scan table, call insertEntry() to put record into index_file

	string indexFileName(tableName+ "_" + attributeName);

	RC rc = 0;
	rc = IndexManager::instance()->createFile(indexFileName);

	//file already exist
	if(rc != 0)
		return -1;


	int indexAttributeLength = 0;
	Attribute indexAttribute;
	vector<Attribute> tableAttributes;
	rc = getAttributes(tableName, tableAttributes);

	// searching attribute
	for(int i= 0 ; i < tableAttributes.size(); i++){
		if(tableAttributes[i].name == attributeName){
			indexAttribute = tableAttributes[i];
			indexAttributeLength = PAGE_SIZE;
//			indexAttributeLength = tableAttributes[i].length;
//
//			//length before the real data
//			if(tableAttributes[i].type == TypeVarChar){
//				indexAttributeLength = indexAttributeLength + 4;
//			}

			break;  //find, can stop searching
		}
	}

	// no attribute found
	if(indexAttributeLength == 0)
		return - 1;

	//init scan_iterator
	RM_ScanIterator rm_ScanIterator;
	vector<string> attrs;
	attrs.push_back(attributeName);
	rc = scan(tableName, "", NO_OP, NULL, attrs, rm_ScanIterator);
	if(rc != 0)
		return -1;

	RID rid;
	void* data = malloc(indexAttributeLength);

	IXFileHandle ixFileHandle;
	IndexManager::instance()->openFile(indexFileName, ixFileHandle);

	RID preRID;
	preRID.pageNum = 0;
	preRID.slotNum = 0;
	//build index file
	while(rm_ScanIterator.getNextTuple(rid, data) != RM_EOF){
		//IndexManager::instance()->insertEntry(ixFileHandle, indexAttribute, data, rid);
		IndexManager::instance()->insertEntry(ixFileHandle, indexAttribute, data, preRID);
		preRID.pageNum = rid.pageNum;
		preRID.slotNum = rid.slotNum;
	}
	IndexManager::instance()->closeFile(ixFileHandle);
	return 0;
}

RC RelationManager::destroyIndex(const string &tableName, const string &attributeName)
{
	string indexFileName(tableName+ "_" + attributeName);

	return IndexManager::instance()->destroyFile(indexFileName);
}

RC RelationManager::indexScan(const string &tableName,
                      const string &attributeName,
                      const void *lowKey,
                      const void *highKey,
                      bool lowKeyInclusive,
                      bool highKeyInclusive,
                      RM_IndexScanIterator &rm_IndexScanIterator)
{
	string indexName(tableName+ "_" + attributeName);
	IXFileHandle ixFileHandle;

	RC rc = 0;
	rc = IndexManager::instance()->openFile(indexName, ixFileHandle);

	//file already exist
	if(rc != 0)	return -1;

//	//rebuild
//	rc = IndexManager::instance()->closeFile(ixFileHandle);
//	destroyIndex(tableName, attributeName);
//	createIndex(tableName, attributeName);
//	rc = IndexManager::instance()->openFile(indexName, ixFileHandle);

	vector<Attribute> attrs;
	rc = getAttributes(tableName, attrs);
	if(rc != 0)	return -1;

	//find match attr
	Attribute attr;
	bool isFind = false;
	for(int i = 0 ; i < attrs.size(); i++){
		if(attrs[i].name == attributeName){
			attr = attrs[i];
			isFind = true;
			break;
		}
	}

	if(!isFind)	return -1;

	/*
	  (IXFileHandle &ixfileHandle,
                const Attribute &attribute,
                const void *lowKey,
                const void *highKey,
                bool lowKeyInclusive,
                bool highKeyInclusive,
                IX_ScanIterator &ix_ScanIterator);
	 * */

	rc = IndexManager::instance()->scan(ixFileHandle, attr, lowKey, highKey, lowKeyInclusive, highKeyInclusive, rm_IndexScanIterator.ix_scanIterator);
//	rc = IndexManager::instance()->closeFile(ixFileHandle);
	return rc;
}


