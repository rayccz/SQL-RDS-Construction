#include <fstream>
#include <iostream>

#include <vector>

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "qe_test_util.h"

int testCase_10() {
	// Mandatory for all
	// 1. Filter
	// 2. Project
	// 3. INLJoin
	// SELECT A1.A, A1.C, right.* FROM (SELECT * FROM left WHERE left.B < 75) A1, right WHERE A1.C = right.C
	cerr << endl << "***** In QE Test Case 10 *****" << endl;

	RC rc = success;

//	rc = RelationManager::instance()->destroyIndex("left", "B");
//	rc = RelationManager::instance()->createIndex("left", "B");

	// Create Filter
	IndexScan *leftIn = new IndexScan(*rm, "left", "B");
//	TableScan *leftIn = new TableScan(*rm, "left");
//	void *ggData = malloc(PAGE_SIZE);
//	vector<Attribute> attrs;
//	leftIn->getAttributes(attrs);
//	while (leftIn->getNextTuple(ggData) != QE_EOF) {
//
//		RecordBasedFileManager::instance()->printRecord(attrs, ggData);
//	}
//	return 0;


//	FileHandle fileHandle;
//	PagedFileManager::instance()->openFile("left_C", fileHandle);
//	void* pageData = malloc(4096);
//	fileHandle.readPage(0, pageData);
//	short slotInfo[2];
//	memcpy(slotInfo, ((char*)pageData) + 4080, sizeof(short) * 2);
//
//	// indicator(1byte) + fieldCount(4bytes) + offset(2 + 2 + 2 bytes) + null_indicator (1byte)
//	int offset = 1 + 4 + 6 + 1;
//	int key;
//	memcpy(&key, ((char*)pageData) + slotInfo[0] + offset, sizeof(int));
//	cout << "key:" << key << endl;
//	free(pageData);
//	PagedFileManager::instance()->closeFile(fileHandle);

	int compVal = 75;

	Condition cond_f;
	cond_f.lhsAttr = "left.B";
	cond_f.op = LT_OP;
	cond_f.bRhsIsAttr = false;
	Value value;
	value.type = TypeInt;
	value.data = malloc(bufSize);
	*(int *) value.data = compVal;
	cond_f.rhsValue = value;

	leftIn->setIterator(NULL, value.data, true, false);
//	leftIn->setIterator();
	Filter *filter = new Filter(leftIn, cond_f); //left.B: 10~74, left.C: 50.0~114.0

//	filter->getAttributes(attrs);
//	while (filter->getNextTuple(ggData) != QE_EOF) {
//		RecordBasedFileManager::instance()->printRecord(attrs, ggData);
//	}
//
//	return 0;

	void *ggData = malloc(PAGE_SIZE);
	vector<Attribute> attrs;
	leftIn->getAttributes(attrs);
	while (leftIn->getNextTuple(ggData) != QE_EOF) {
		RecordBasedFileManager::instance()->printRecord(attrs, ggData);
	}
	return 0;


	// Create Project
	vector<string> attrNames;
	attrNames.push_back("left.A");
	attrNames.push_back("left.C");
	Project *project = new Project(filter, attrNames);

	Condition cond_j;
	cond_j.lhsAttr = "left.C";
	cond_j.op = EQ_OP;
	cond_j.bRhsIsAttr = true;
	cond_j.rhsAttr = "right.C";

	// Create Join
	IndexScan *rightIn = NULL;
	Iterator *join = NULL;
	rightIn = new IndexScan(*rm, "right", "C");
	join = new INLJoin(project, rightIn, cond_j);

	int expectedResultCnt = 65; //50.0~114.0
	int actualResultCnt = 0;
	float valueC = 0;

	// Go over the data through iterator
	void *data = malloc(bufSize);
	bool nullBit = false;

	while (join->getNextTuple(data) != QE_EOF) {
		int offset = 0;

		// Is an attribute left.A NULL?
		nullBit = *(unsigned char *)((char *)data) & (1 << 7);
		if (nullBit) {
			cerr << endl << "***** A returned value is not correct. *****" << endl;
			goto clean_up;
		}
		// Print left.A
		cerr << "left.A " << *(int *) ((char *) data + offset + 1);
		offset += sizeof(int);

		// Is an attribute left.C NULL?
		nullBit = *(unsigned char *)((char *)data) & (1 << 6);
		if (nullBit) {
			cerr << endl << "***** A returned value is not correct. *****" << endl;
			goto clean_up;
		}
		// Print left.C
		cerr << "  left.C " << *(float *) ((char *) data + offset + 1);
		offset += sizeof(float);

		// Is an attribute right.B NULL?
		nullBit = *(unsigned char *)((char *)data) & (1 << 5);
		if (nullBit) {
			cerr << endl << "***** A returned value is not correct. *****" << endl;
			goto clean_up;
		}
		// Print right.B
		cerr << "  right.B " << *(int *) ((char *) data + offset + 1);
		offset += sizeof(int);

		// Is an attribute right.C NULL?
		nullBit = *(unsigned char *)((char *)data) & (1 << 4);
		if (nullBit) {
			cerr << endl << "***** A returned value is not correct. *****" << endl;
			goto clean_up;
		}
		// Print right.C
		valueC = *(float *) ((char *) data + offset + 1);
		cerr << "  right.C " << valueC;
		offset += sizeof(float);
		if (valueC < 50.0 || valueC > 114.0) {
			rc = fail;
			goto clean_up;
		}

		// Is an attribute right.D NULL?
		nullBit = *(unsigned char *)((char *)data) & (1 << 3);
		if (nullBit) {
			cerr << endl << "***** A returned value is not correct. *****" << endl;
			goto clean_up;
		}
		// Print right.D
		cerr << "  right.D " << *(int *) ((char *) data + offset + 1);
		offset += sizeof(int);
		cout << endl;
		memset(data, 0, bufSize);
		actualResultCnt++;
	}
	cout << endl;
	if (expectedResultCnt != actualResultCnt) {
//		cout << "actualResultCnt:" << actualResultCnt << endl;
		cerr << "***** The number of returned tuple is not correct. *****" << endl;
		rc = fail;
	}

clean_up:
	delete join;
	delete rightIn;
	delete project;
	delete filter;
	delete leftIn;
	free(value.data);
	free(data);
	return rc;
}


int main() {

	if (testCase_10() != success) {
		cerr << "***** [FAIL] QE Test Case 10 failed. *****" << endl;
		return fail;
	} else {
		cerr << "***** QE Test Case 10 finished. The result will be examined. *****" << endl;
		return success;
	}
}
