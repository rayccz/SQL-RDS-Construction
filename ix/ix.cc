
#include "ix.h"
#include <stack>

IndexManager* IndexManager::_index_manager = 0;

IndexManager* IndexManager::instance()
{
    if(!_index_manager)
        _index_manager = new IndexManager();

    return _index_manager;
}

IndexManager::IndexManager()
{
}

IndexManager::~IndexManager()
{
}

RC IndexManager::createFile(const string &fileName)
{
	string s = fileName + "_rootpage";
	PagedFileManager::instance()->createFile(s);
	return PagedFileManager::instance()->createFile(fileName);
}

RC IndexManager::destroyFile(const string &fileName)
{
	string s = fileName + "_rootpage";
	PagedFileManager::instance()->createFile(s);
	return PagedFileManager::instance()->destroyFile(fileName);
}

RC IndexManager::openFile(const string &fileName, IXFileHandle &ixfileHandle)
{
	if (ixfileHandle.used == 1){
		return -1;
	}
	ixfileHandle.used=1;
	return PagedFileManager::instance()->openFile(fileName, ixfileHandle.fileHandle);
}

RC IndexManager::closeFile(IXFileHandle &ixfileHandle)
{
	return PagedFileManager::instance()->closeFile(ixfileHandle.fileHandle);
	ixfileHandle.used=0;
}

void createIfLeafDescriptor(vector<Attribute> &recordDescriptor) {
    Attribute attr;
    attr.name = "IfLeaf";
    attr.type = TypeInt;
    attr.length = (AttrLength)4;
    recordDescriptor.push_back(attr);
}

void createNextPagePointerDescriptor(vector<Attribute> &recordDescriptor) {
    Attribute attr;
    attr.name = "NextPagePointer";
    attr.type = TypeInt;
    attr.length = (AttrLength)4;
    recordDescriptor.push_back(attr);
}

void createKeyRidDescriptor(vector<Attribute> &recordDescriptor) {
    Attribute attr;

    attr.name = "RidPage";
	attr.type = TypeInt;
	attr.length = (AttrLength)4;
	recordDescriptor.push_back(attr);

	attr.name = "RidSlot";
	attr.type = TypeInt;
	attr.length = (AttrLength)4;
	recordDescriptor.push_back(attr);
}

void createFixedLeftPointerDescriptor(vector<Attribute> &recordDescriptor) {
    Attribute attr;
    attr.name = "FixedLeftPointer";
    attr.type = TypeInt;
    attr.length = (AttrLength)4;
    recordDescriptor.push_back(attr);
}

void createKeyPointerDescriptor(vector<Attribute> &recordDescriptor) { // the key descriptor will be add in function
    Attribute attr;
    attr.name = "RightPointer";
    attr.type = TypeInt;
    attr.length = (AttrLength)4;
    recordDescriptor.push_back(attr);
}

void prepareIfLeafRecord(int fieldCount, unsigned char *nullFieldsIndicator,
		const int IfLeaf, void *buffer,
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
		memcpy((char *) buffer + offset, &IfLeaf, sizeof(int));
		offset += sizeof(int);
	}

	*recordSize = offset;
}

void prepareNextPagePointerRecord(int fieldCount, unsigned char *nullFieldsIndicator,
		const int NextPagePointer, void *buffer,
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
		memcpy((char *) buffer + offset, &NextPagePointer, sizeof(int));
		offset += sizeof(int);
	}

	*recordSize = offset;
}

void prepareKeyRidRecord(int fieldCount, unsigned char *nullFieldsIndicator,
		const Attribute &attribute, const void *key, const int RidPage, const int RidSlot,
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

	if (attribute.type == TypeInt){
		// int
		nullBit = nullFieldsIndicator[0] & (1 << 7);
		if (!nullBit) {
			//int test;
			//memcpy(&test, ((char *) key), sizeof(int));
			memcpy((char *) buffer + offset, ((char *) key), sizeof(int));
			offset += sizeof(int);
		}
		// int
		nullBit = nullFieldsIndicator[0] & (1 << 6);
		if (!nullBit) {
			memcpy((char *) buffer + offset, &RidPage, sizeof(int));
			offset += sizeof(int);
		}
		// int
		nullBit = nullFieldsIndicator[0] & (1 << 5);
		if (!nullBit) {
			memcpy((char *) buffer + offset, &RidSlot, sizeof(int));
			offset += sizeof(int);
		}
	}
	else if (attribute.type == TypeReal){
		// real
		nullBit = nullFieldsIndicator[0] & (1 << 7);
		if (!nullBit) {
			memcpy((char *) buffer + offset, ((char *) key), sizeof(float));
			offset += sizeof(float);
		}
		// int
		nullBit = nullFieldsIndicator[0] & (1 << 6);
		if (!nullBit) {
			memcpy((char *) buffer + offset, &RidPage, sizeof(int));
			offset += sizeof(int);
		}
		// int
		nullBit = nullFieldsIndicator[0] & (1 << 5);
		if (!nullBit) {
			memcpy((char *) buffer + offset, &RidSlot, sizeof(int));
			offset += sizeof(int);
		}
	}
	else{
		int length;
		memcpy(&length, (char *) key, sizeof(int));
		// char
		nullBit = nullFieldsIndicator[0] & (1 << 7);
		if (!nullBit) {
			memcpy((char *) buffer + offset, &length, sizeof(int));
			offset += sizeof(int);
			memcpy((char *) buffer + offset, (char *) key + 4, length);
			offset += length;
		}
		// int
		nullBit = nullFieldsIndicator[0] & (1 << 6);
		if (!nullBit) {
			memcpy((char *) buffer + offset, &RidPage, sizeof(int));
			offset += sizeof(int);
		}
		// int
		nullBit = nullFieldsIndicator[0] & (1 << 5);
		if (!nullBit) {
			memcpy((char *) buffer + offset, &RidSlot, sizeof(int));
			offset += sizeof(int);
		}
	}
	*recordSize = offset;
}

void prepareFixedLeftPointerRecord(int fieldCount, unsigned char *nullFieldsIndicator,
		const int FixedLeftPointer, void *buffer,
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
		memcpy((char *) buffer + offset, &FixedLeftPointer, sizeof(int));
		offset += sizeof(int);
	}

	*recordSize = offset;
}

void prepareKeyPointerRecord(int fieldCount, unsigned char *nullFieldsIndicator,
		const Attribute &attribute, const void *key, const int rightpointer,
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

	if (attribute.type == TypeInt){
		// int
		nullBit = nullFieldsIndicator[0] & (1 << 7);
		if (!nullBit) {
			//int test;
			//memcpy(&test, ((char *) key), sizeof(int));
			memcpy((char *) buffer + offset, ((char *) key), sizeof(int));
			offset += sizeof(int);
		}
		// int
		nullBit = nullFieldsIndicator[0] & (1 << 6);
		if (!nullBit) {
			memcpy((char *) buffer + offset, &rightpointer, sizeof(int));
			offset += sizeof(int);
		}
	}
	else if (attribute.type == TypeReal){
		// real
		nullBit = nullFieldsIndicator[0] & (1 << 7);
		if (!nullBit) {
			memcpy((char *) buffer + offset, ((char *) key), sizeof(float));
			offset += sizeof(float);
		}
		// int
		nullBit = nullFieldsIndicator[0] & (1 << 6);
		if (!nullBit) {
			memcpy((char *) buffer + offset, &rightpointer, sizeof(int));
			offset += sizeof(int);
		}
	}
	else{
		int length;
		memcpy(&length, (char *) key, sizeof(int));
		// char
		nullBit = nullFieldsIndicator[0] & (1 << 7);
		if (!nullBit) {
			memcpy((char *) buffer + offset, &length, sizeof(int));
			offset += sizeof(int);
			memcpy((char *) buffer + offset, (char *) key + 4, length);
			offset += length;
		}
		// int
		nullBit = nullFieldsIndicator[0] & (1 << 6);
		if (!nullBit) {
			memcpy((char *) buffer + offset, &rightpointer, sizeof(int));
			offset += sizeof(int);
		}
	}
	*recordSize = offset;
}

int compare(const Attribute &attribute, int slot, const void *key, void *pageData){ // return 0 if key = compare slot, return 1 if key > compare slot, return -1 if key < compare slot
	slot = slot + 1;
	if (attribute.type == TypeInt){
		short addr;
		memcpy(&addr, ((char*) pageData) + 4096 - 4 - (slot * sizeof(short) * 2), sizeof(short));

		int tmp;
		memcpy(&tmp, ((char*) pageData) + addr + 12, sizeof(int));
		int tmpkey;
		memcpy(&tmpkey, (char*) key, sizeof(int));

		if (tmpkey == tmp){
			return 0;
		}
		else if (tmpkey > tmp){
			return 1;
		}
		else{
			return -1;
		}
	}
	else if (attribute.type == TypeReal){
		short addr;
		memcpy(&addr, ((char*) pageData) + 4096 - 4 - (slot * sizeof(short) * 2), sizeof(short));

		float tmp;
		memcpy(&tmp, ((char*) pageData) + addr + 12, sizeof(float));
		float tmpkey;
		memcpy(&tmpkey, (char*) key, sizeof(float));

		if (tmpkey == tmp){
			return 0;
		}
		else if (tmpkey > tmp){
			return 1;
		}
		else{
			return -1;
		}
	}
	else{
		short addr;
		memcpy(&addr, ((char*) pageData) + 4096 - 4 - (slot * sizeof(short) * 2), sizeof(short));
		int tmplength;
		memcpy(&tmplength, ((char*) pageData) + addr + 12, sizeof(int));
		string tmp="";
		for (int i=0;i<tmplength;i++){
			char c;
			memcpy(&c, ((char*) pageData) + addr + 12 + 4 +i, sizeof(char));
			tmp = tmp + c;
		}
		int tmpkeylength;
		memcpy(&tmpkeylength, ((char*) key), sizeof(int));
		string tmpkey = "";
		for (int i=0;i<tmpkeylength;i++){
			char c;
			memcpy(&c, ((char*) key) + 4 + i, sizeof(char));
			tmpkey = tmpkey + c;
		}

		if (tmpkey == tmp){
			return 0;
		}
		else if (tmpkey > tmp){
			return 1;
		}
		else{
			return -1;
		}
	}
}

void KeyRidContinuousUpdate(IXFileHandle &ixfileHandle, int N, int insert_rid, const void *pageData, const vector<Attribute> &KeyRidDescriptor, const void *KeyRidrecord, int page){
	int rest = N - insert_rid; // the number of rest slot which need to be updated
	int slotupdate=insert_rid; // the slot that need to be update
	void *KeyRidrecord_tmp = malloc(4096); // the record that be reserved temperately
	void *KeyRidrecord_tmp2 = malloc(4096); // the record that be reserved temperately

	while(rest > 0){
		// get the record that need to be updated
		short addr;
		memcpy(&addr, ((char*) pageData) + 4096 - 4 - ((slotupdate+1) * sizeof(short) * 2), sizeof(short));
		short len;
		memcpy(&len, ((char*) pageData) + 4096 - 4 - ((slotupdate+1) * sizeof(short) * 2) + 2, sizeof(short));
		memcpy(((char*) KeyRidrecord_tmp), ((char*) pageData) + addr + 11, len - 11); // 11 = 1 + 4 + 2 + 2 + 2 one indicator + offset + 3*slot

		if (rest == N - insert_rid){ // first time in  the loop
			// update the previous key with the current key
			RID rid;
			rid.pageNum = page;
			rid.slotNum = slotupdate;
			RecordBasedFileManager::instance()->updateRecord(ixfileHandle.fileHandle, KeyRidDescriptor, KeyRidrecord, rid);
		}
		else{
			RID rid;
			rid.pageNum = page;
			rid.slotNum = slotupdate;
			RecordBasedFileManager::instance()->updateRecord(ixfileHandle.fileHandle, KeyRidDescriptor, KeyRidrecord_tmp2, rid);
		}
		memcpy((char*) KeyRidrecord_tmp2, (char*) KeyRidrecord_tmp, 4096); // assign the KeyRidrecord_tmp2 with KeyRidrecord_tmp
		slotupdate++;
		rest = rest - 1;
	}
	RID rid;
	RecordBasedFileManager::instance()->IXinsertRecord(ixfileHandle.fileHandle, KeyRidDescriptor, KeyRidrecord_tmp, rid, page);
	free(KeyRidrecord_tmp);
	free(KeyRidrecord_tmp2);
}

void KeyPointerContinuousUpdate(IXFileHandle &ixfileHandle, int N, int insert_rid, const void *pageData, const vector<Attribute> &KeyPointerDescriptor, const void *KeyPointerrecord, int page){
	int rest = N - insert_rid; // the number of rest slot which need to be updated
	int slotupdate=insert_rid; // the slot that need to be update
	void *KeyPointerrecord_tmp = malloc(4096); // the record that be reserved temperately
	void *KeyPointerrecord_tmp2 = malloc(4096); // the record that be reserved temperately

	while(rest > 0){
		// get the record that need to be updated
		short addr;
		memcpy(&addr, ((char*) pageData) + 4096 - 4 - ((slotupdate+1) * sizeof(short) * 2), sizeof(short));
		short len;
		memcpy(&len, ((char*) pageData) + 4096 - 4 - ((slotupdate+1) * sizeof(short) * 2) + 2, sizeof(short));
		memcpy(((char*) KeyPointerrecord_tmp), ((char*) pageData) + addr + 9, len - 9); // 11 = 1 + 4 + 2 + 2 one indicator + offset + 2*slot

		if (rest == N - insert_rid){ // first time in  the loop
			// update the previous key with the current key
			RID rid;
			rid.pageNum = page;
			rid.slotNum = slotupdate;
			RecordBasedFileManager::instance()->updateRecord(ixfileHandle.fileHandle, KeyPointerDescriptor, KeyPointerrecord, rid);
		}
		else{
			RID rid;
			rid.pageNum = page;
			rid.slotNum = slotupdate;
			RecordBasedFileManager::instance()->updateRecord(ixfileHandle.fileHandle, KeyPointerDescriptor, KeyPointerrecord_tmp2, rid);
		}
		memcpy((char*) KeyPointerrecord_tmp2, (char*) KeyPointerrecord_tmp, 4096); // assign the KeyRidrecord_tmp2 with KeyRidrecord_tmp
		slotupdate++;
		rest = rest - 1;
	}
	RID rid;
	RecordBasedFileManager::instance()->IXinsertRecord(ixfileHandle.fileHandle, KeyPointerDescriptor, KeyPointerrecord_tmp, rid, page);
	free(KeyPointerrecord_tmp);
	free(KeyPointerrecord_tmp2);
}

void BinarySearchInsert(IXFileHandle &ixfileHandle, const vector<Attribute> &KeyRidDescriptor, const void *KeyRidrecord, int N, const Attribute &attribute, const void *key, void *pageData, int page){
	int end = 2;
	int begin = N -1;
	RID insert_rid;
	int mid = 0;

	while (begin != end and begin > end){
		mid = (begin + end)/2;
		if(compare(attribute, mid, key, pageData) == 0){ // key == compare slot
			begin = mid;
			end = begin;
		}
		else if (compare(attribute, mid, key, pageData) == 1){ // key > compare slot
			end = mid + 1;
		}
		else{ // key < compare slot
			begin = mid - 1;
		}
	}
	// derive the rid
	insert_rid.pageNum = page;
	insert_rid.slotNum = end;

	if(compare(attribute, insert_rid.slotNum, key, pageData) >= 0){ // the key is >= the compare slot
		// Directly insert the key if it will be insert at the last place
		if (N == insert_rid.slotNum + 1){
			RID rid;
			RecordBasedFileManager::instance()->IXinsertRecord(ixfileHandle.fileHandle, KeyRidDescriptor, KeyRidrecord, rid, insert_rid.pageNum);
		}
		// the continuous update need to be done
		else{
			KeyRidContinuousUpdate(ixfileHandle, N, insert_rid.slotNum+1, pageData, KeyRidDescriptor, KeyRidrecord, insert_rid.pageNum); // update start from insert_rid+1 slot
		}
	}
	else{ // the key is < the compare slot so it need to be insert in front of it
		KeyRidContinuousUpdate(ixfileHandle, N, insert_rid.slotNum, pageData, KeyRidDescriptor, KeyRidrecord, insert_rid.pageNum); // update start from insert_rid slot
	}
}

int key_nodekey_compare(const Attribute &attribute, const void *key, void *nodekey){ // return 0 if key = nodekey, return 1 if key > nodekey, return -1 if key < nodekey
	if (attribute.type == TypeInt){

		int tmp;
		memcpy(&tmp, ((char*) nodekey), sizeof(int));
		int tmpkey;
		memcpy(&tmpkey, (char*) key, sizeof(int));

		if (tmpkey == tmp){
			return 0;
		}
		else if (tmpkey > tmp){
			return 1;
		}
		else{
			return -1;
		}
	}
	else if (attribute.type == TypeReal){

		float tmp;
		memcpy(&tmp, ((char*) nodekey), sizeof(float));
		float tmpkey;
		memcpy(&tmpkey, (char*) key, sizeof(float));

		if (tmpkey == tmp){
			return 0;
		}
		else if (tmpkey > tmp){
			return 1;
		}
		else{
			return -1;
		}
	}
	else{
		int tmplength;
		memcpy(&tmplength, ((char*) nodekey), sizeof(int));
		string tmp="";
		for (int i=0;i<tmplength;i++){
			char c;
			memcpy(&c, ((char*) nodekey) + 4 + i, sizeof(char));
			tmp = tmp + c;
		}
		int tmpkeylength;
		memcpy(&tmpkeylength, ((char*) key), sizeof(int));
		string tmpkey = "";
		for (int i=0;i<tmpkeylength;i++){
			char c;
			memcpy(&c, ((char*) key) + 4 + i, sizeof(char));
			tmpkey = tmpkey + c;
		}

		if (tmpkey == tmp){
			return 0;
		}
		else if (tmpkey > tmp){
			return 1;
		}
		else{
			return -1;
		}
	}
}

void keynodeInsert(IXFileHandle &ixfileHandle, const vector<Attribute> &KeyPointerDescriptor, const void *KeyPointerrecord, int N, const Attribute &attribute, const void *key, void *pageData, int page, int slot){
	// derive the rid
	RID insert_rid;
	insert_rid.pageNum = page;
	insert_rid.slotNum = slot;

	void *KeyPointer = malloc(4096); // the record that be reserved temperately
	short addr;
	memcpy(&addr, ((char*) pageData) + 4096 - 4 - ((slot+1) * sizeof(short) * 2), sizeof(short));
	short slot2;
	memcpy(&slot2, ((char*) pageData) + addr + 1 + 4, sizeof(short));
	memcpy(((char*) KeyPointer), ((char*) pageData) + addr + 10, slot2 -10); // 1 + 4 + 2 + 2 + 1

	if(key_nodekey_compare(attribute, key, KeyPointer) >= 0){ // the key is >= the compare slot
		// Directly insert the key if it will be insert at the last place
		if (N == insert_rid.slotNum + 1){
			RID rid;
			RecordBasedFileManager::instance()->IXinsertRecord(ixfileHandle.fileHandle, KeyPointerDescriptor, KeyPointerrecord, rid, insert_rid.pageNum);
		}
		// the continuous update need to be done
		else{
			KeyPointerContinuousUpdate(ixfileHandle, N, insert_rid.slotNum+1, pageData, KeyPointerDescriptor, KeyPointerrecord, insert_rid.pageNum); // update start from insert_rid+1 slot
		}
	}
	else{ // the key is < the compare slot so it need to be insert in front of it
		KeyPointerContinuousUpdate(ixfileHandle, N, insert_rid.slotNum, pageData, KeyPointerDescriptor, KeyPointerrecord, insert_rid.pageNum); // update start from insert_rid slot
	}
	free(KeyPointer);
}

int find_the_replace_node(const Attribute &attribute, int N, void *mid_key, void *pageData){
	int rest = N - 2;
	int start = 2;
	int replace;
	while(rest > 0){

		void *KeyPointer = malloc(4096); // the record that be reserved temperately
		short addr;
		memcpy(&addr, ((char*) pageData) + 4096 - 4 - ((start+1) * sizeof(short) * 2), sizeof(short));
		short slot;
		memcpy(&slot, ((char*) pageData) + addr + 1 + 4, sizeof(short));
		memcpy(((char*) KeyPointer), ((char*) pageData) + addr + 10, slot -10); // 1 + 4 + 2 + 2 + 1

		if(key_nodekey_compare(attribute, mid_key, KeyPointer) == 0){// return 0 if key = nodekey, return 1 if key > nodekey, return -1 if key < nodekey
			replace = start;
			break;
		}
		else if(key_nodekey_compare(attribute, mid_key, KeyPointer) == 1){
			if (start == N - 1){ // last key
				replace = start;
				break;
			}
		}
		else{
			replace = start;
			break;
		}
		free(KeyPointer);
		start++;
		rest--;
	}
	return replace;
}

void testprint(void *pageData){
	short NF[2];
	memcpy(NF, ((char*) pageData) + 4092, sizeof(short) * 2);
	int isleaf;
	memcpy(&isleaf, ((char*) pageData) + 1 + 4 + 2 + 1, sizeof(int));
	int pointer;
	memcpy (&pointer, ((char*) pageData) + 12 + 1 + 4 + 2 + 1, sizeof(int));
	cout<<"isleaf: "<< isleaf << endl;
	cout<< "pointer: "<< pointer <<endl;
	int rest = NF[0] - 2;
	int start = 2;
	while (rest > 0){
		short addr;
		memcpy(&addr, ((char*) pageData) + 4096 - 4 - ((start+1) * sizeof(short) * 2), sizeof(short));
		short len;
		memcpy(&len, ((char*) pageData) + 4096 - 4 - ((start+1) * sizeof(short) * 2) + 2, sizeof(short));
		int tmp;
		memcpy(&tmp, ((char*) pageData) + addr + 12, 4); // 12 = 1 + 4 + 2 + 2 + 2 + 1 one indicator + offset + 3*slot + 1
		start ++ ;
		rest --;
		cout<< "key: "<< tmp<<endl;
	}
}

void chartestprint(void *pageData){
	short NF[2];
	memcpy(NF, ((char*) pageData) + 4092, sizeof(short) * 2);
	int isleaf;
	memcpy(&isleaf, ((char*) pageData) + 1 + 4 + 2 + 1, sizeof(int));
	int pointer;
	memcpy (&pointer, ((char*) pageData) + 12 + 1 + 4 + 2 + 1, sizeof(int));
	cout<<"isleaf: "<< isleaf << endl;
	cout<< "pointer: "<< pointer <<endl;
	int rest = NF[0] - 2;
	int start = 2;
	while (rest > 0){
		short addr;
		memcpy(&addr, ((char*) pageData) + 4096 - 4 - ((start+1) * sizeof(short) * 2), sizeof(short));
		short len;
		memcpy(&len, ((char*) pageData) + addr + 1 + 4, sizeof(short));
		void* tmp = malloc(4096);
		memcpy(((char*)tmp), ((char*) pageData) + addr + 12, len-12); // 12 = 1 + 4 + 2 + 2 + 2 +1 one indicator + offset + 3*slot
		int tmpkeylength;
		memcpy(&tmpkeylength, ((char*) tmp), sizeof(int));
		string tmpkey = "";
		for (int i=0;i<tmpkeylength;i++){
			char c;
			memcpy(&c, ((char*) tmp) + 4 + i, sizeof(char));
			tmpkey = tmpkey + c;
		}
		start ++ ;
		rest --;
		cout<< "key: "<< tmpkey<<endl;
		free(tmp);
	}
}

void testnodeprint(void *pageData){
	short NF[2];
	memcpy(NF, ((char*) pageData) + 4092, sizeof(short) * 2);
	int isleaf;
	memcpy(&isleaf, ((char*) pageData) + 1 + 4 + 2 + 1, sizeof(int));
	int pointer;
	memcpy (&pointer, ((char*) pageData) + 12 + 1 + 4 + 2 + 1, sizeof(int));
	cout<<"isleaf: "<< isleaf << endl;
	cout<< "left pointer: "<< pointer <<endl;
	int rest = NF[0] - 2;
	int start = 2;
	while (rest > 0){
		short addr;
		memcpy(&addr, ((char*) pageData) + 4096 - 4 - ((start+1) * sizeof(short) * 2), sizeof(short));
		short len;
		memcpy(&len, ((char*) pageData) + 4096 - 4 - ((start+1) * sizeof(short) * 2) + 2, sizeof(short));
		int tmp;
		memcpy(&tmp, ((char*) pageData) + addr + 10, 4); // 10 = 1 + 4 + 2 + 2 + 1one indicator + offset + 2*slot
		memcpy(&pointer, ((char*) pageData) + addr + 10 + 4, 4);
		start ++ ;
		rest --;
		cout<< "key: "<< tmp << " pointer: " << pointer <<endl;
	}
}

void chartestnodeprint(void *pageData){
	short NF[2];
	memcpy(NF, ((char*) pageData) + 4092, sizeof(short) * 2);
	int isleaf;
	memcpy(&isleaf, ((char*) pageData) + 1 + 4 + 2 + 1, sizeof(int));
	int pointer;
	memcpy (&pointer, ((char*) pageData) + 12 + 1 + 4 + 2 + 1, sizeof(int));
	cout<<"isleaf: "<< isleaf << endl;
	cout<< "left pointer: "<< pointer <<endl;
	int rest = NF[0] - 2;
	int start = 2;
	while (rest > 0){
		short addr;
		memcpy(&addr, ((char*) pageData) + 4096 - 4 - ((start+1) * sizeof(short) * 2), sizeof(short));
		short len;
		memcpy(&len, ((char*) pageData) + addr + 1 + 4, sizeof(short));


		void* tmp = malloc(4096);
		memcpy(((char*)tmp), ((char*) pageData) + addr + 10, len-10); // 10 = 1 + 4 + 2 + 2 +1 one indicator + offset + 2*slot
		int tmpkeylength;
		memcpy(&tmpkeylength, ((char*) tmp), sizeof(int));
		string tmpkey = "";
		for (int i=0;i<tmpkeylength;i++){
			char c;
			memcpy(&c, ((char*) tmp) + 4 + i, sizeof(char));
			tmpkey = tmpkey + c;
		}
		memcpy(&pointer, ((char*) pageData) + addr + 10 + 4 + tmpkeylength, 4);
		start ++ ;
		rest --;
		cout<< "key: "<< tmpkey<< " pointer: " << pointer << endl;
		free(tmp);
	}
}

RC IndexManager::insertEntry(IXFileHandle &ixfileHandle, const Attribute &attribute, const void *key, const RID &rid)
{
	// prepare the key record
	vector<Attribute> KeyRidDescriptor;
	KeyRidDescriptor.push_back(attribute);
	createKeyRidDescriptor(KeyRidDescriptor);

	int KeyRidnullFieldsIndicatorActualSize = ceil((double) KeyRidDescriptor.size() / CHAR_BIT);
	unsigned char *KeyRidnullsIndicator = (unsigned char *) malloc(KeyRidnullFieldsIndicatorActualSize);
	memset(KeyRidnullsIndicator, 0, KeyRidnullFieldsIndicatorActualSize);
	int KeyRidrecordSize = 0;
	void *KeyRidrecord = malloc(4096);
	prepareKeyRidRecord(KeyRidDescriptor.size(), KeyRidnullsIndicator, attribute, key, rid.pageNum, rid.slotNum, KeyRidrecord, &KeyRidrecordSize);
	free(KeyRidnullsIndicator);

	if(ixfileHandle.fileHandle.getNumberOfPages() == 0){ // the file has not been written
		//initialize a leaf
		vector<Attribute> IfLeafDescriptor;
		createIfLeafDescriptor(IfLeafDescriptor);
		int IfLeafnullFieldsIndicatorActualSize = ceil((double) IfLeafDescriptor.size() / CHAR_BIT);
		unsigned char *IfLeafnullsIndicator = (unsigned char *) malloc(IfLeafnullFieldsIndicatorActualSize);
		memset(IfLeafnullsIndicator, 0, IfLeafnullFieldsIndicatorActualSize);
		int IfLeafrecordSize = 0;
		void *IfLeafrecord = malloc(4096);
		prepareIfLeafRecord(IfLeafDescriptor.size(), IfLeafnullsIndicator, 1, IfLeafrecord, &IfLeafrecordSize); // the IfLeaf Indicator to indicate if it is a leaf or not
		free(IfLeafnullsIndicator);

		vector<Attribute> NextPagePointerDescriptor;
		createNextPagePointerDescriptor(NextPagePointerDescriptor);
		int NextPagePointernullFieldsIndicatorActualSize = ceil((double) NextPagePointerDescriptor.size() / CHAR_BIT);
		unsigned char *NextPagePointernullsIndicator = (unsigned char *) malloc(NextPagePointernullFieldsIndicatorActualSize);
		memset(NextPagePointernullsIndicator, 0, NextPagePointernullFieldsIndicatorActualSize);
		int NextPagePointerrecordSize = 0;
		void *NextPagePointerrecord = malloc(4096);
		prepareNextPagePointerRecord(NextPagePointerDescriptor.size(), NextPagePointernullsIndicator, -1, NextPagePointerrecord, &NextPagePointerrecordSize); // The page pointer to the next page initial be -1
		free(NextPagePointernullsIndicator);

		RID rid;
		RecordBasedFileManager::instance()->insertRecord(ixfileHandle.fileHandle, IfLeafDescriptor, IfLeafrecord, rid);
		RecordBasedFileManager::instance()->insertRecord(ixfileHandle.fileHandle, NextPagePointerDescriptor, NextPagePointerrecord, rid);
		RecordBasedFileManager::instance()->insertRecord(ixfileHandle.fileHandle, KeyRidDescriptor, KeyRidrecord, rid);

		/*
		int tmplength;
		memcpy(&tmplength, ((char*) KeyRidrecord)+1, sizeof(int));
		string tmp="";
		for (int i=0;i<tmplength;i++){
			char c;
			memcpy(&c, ((char*) KeyRidrecord) +1 + 4 + i, sizeof(char));
			tmp = tmp + c;
		}
		memcpy(&tmplength, ((char*) key), sizeof(int));
		tmp="";
		for (int i=0;i<tmplength;i++){
			char c;
			memcpy(&c, ((char*) key) +4 + i, sizeof(char));
			tmp = tmp + c;
		}
		*/

		string s = ixfileHandle.fileHandle.fileName + "_rootpage";
		FileHandle fileHandle_tmp;
		PagedFileManager::instance()->openFile(s, fileHandle_tmp);
		fileHandle_tmp.appendPage2();
		void* pageData_rootpage = malloc(PAGE_SIZE);
		fileHandle_tmp.readPage(0, pageData_rootpage);
		ixfileHandle.rootpage = 0; //initialize the rootpage
		memcpy(((char*)pageData_rootpage), &ixfileHandle.rootpage, sizeof(int));
		fileHandle_tmp.writePage(0,pageData_rootpage);
		PagedFileManager::instance()->closeFile(fileHandle_tmp);
		free(pageData_rootpage);

		free(IfLeafrecord);
		free(NextPagePointerrecord);
	}
	else{
		//test
		/*
		void* pageDatatest = malloc(PAGE_SIZE);
		ixfileHandle.fileHandle.readPage(9, pageDatatest);
		testnodeprint(pageDatatest);
		cout << endl;
		ixfileHandle.fileHandle.readPage(2, pageDatatest);
		testnodeprint(pageDatatest);
		cout << endl;
		ixfileHandle.fileHandle.readPage(8, pageDatatest);
		testnodeprint(pageDatatest);
		cout << endl;
		ixfileHandle.fileHandle.readPage(3, pageDatatest);
		testprint(pageDatatest);
		cout << endl;
		ixfileHandle.fileHandle.readPage(6, pageDatatest);
		testprint(pageDatatest);
		cout << endl;
		ixfileHandle.fileHandle.readPage(5, pageDatatest);
		testprint(pageDatatest);
		cout << endl;
		ixfileHandle.fileHandle.readPage(1, pageDatatest);
		testprint(pageDatatest);
		cout << endl;
		ixfileHandle.fileHandle.readPage(7, pageDatatest);
		testprint(pageDatatest);
		cout << endl;
		ixfileHandle.fileHandle.readPage(4, pageDatatest);
		testprint(pageDatatest);
		cout << endl;
		ixfileHandle.fileHandle.readPage(0, pageDatatest);
		testprint(pageDatatest);
		free(pageDatatest);
		*/

		/*
		void* pageDatatest = malloc(PAGE_SIZE);
		if(ixfileHandle.fileHandle.getNumberOfPages()>=9){
			ixfileHandle.fileHandle.readPage(8, pageDatatest);
			chartestnodeprint(pageDatatest);
			cout<<endl;
		}

		if(ixfileHandle.fileHandle.getNumberOfPages()>=3){
			ixfileHandle.fileHandle.readPage(2, pageDatatest);
			chartestnodeprint(pageDatatest);
			cout<<endl;
		}

		if(ixfileHandle.fileHandle.getNumberOfPages()>=8){
			ixfileHandle.fileHandle.readPage(7, pageDatatest);
			chartestnodeprint(pageDatatest);
			cout<<endl;
		}

		if(ixfileHandle.fileHandle.getNumberOfPages()>=12){
			ixfileHandle.fileHandle.readPage(11, pageDatatest);
			chartestnodeprint(pageDatatest);
			cout<<endl;
		}

		ixfileHandle.fileHandle.readPage(0, pageDatatest);
		chartestprint(pageDatatest);
		cout<<endl;

		if(ixfileHandle.fileHandle.getNumberOfPages()>=2){
			ixfileHandle.fileHandle.readPage(1, pageDatatest);
			chartestprint(pageDatatest);
			cout<<endl;
		}

		if(ixfileHandle.fileHandle.getNumberOfPages()>=2){
			ixfileHandle.fileHandle.readPage(1, pageDatatest);
			chartestprint(pageDatatest);
			cout<<endl;
		}

		if(ixfileHandle.fileHandle.getNumberOfPages()>=13){
			ixfileHandle.fileHandle.readPage(12, pageDatatest);
			chartestprint(pageDatatest);
			cout<<endl;
		}

		for (int i=3;i<ixfileHandle.fileHandle.getNumberOfPages();i++){
			ixfileHandle.fileHandle.readPage(i, pageDatatest);
			chartestprint(pageDatatest);
			cout<<endl;
		}

		free(pageDatatest);
		*/

		/*
		int t;
		memcpy(&t, (char*) key, sizeof(int));
		if(t%1000==0){
		cout<<t<<endl;
		}
		*/


		/*
		cout<<"///////////////////////////////////////////////////////////////////////////////"<<endl;

		int tmpkeylength;
		memcpy(&tmpkeylength, ((char*) key), sizeof(int));
		string tmpkey = "";
		for (int i=0;i<tmpkeylength;i++){
			char c;
			memcpy(&c, ((char*) key) + 4, sizeof(char));
			tmpkey = tmpkey + c;
		}
		cout<< "key: "<< tmpkey<<endl;

		cout<<"///////////////////////////////////////////////////////////////////////////////"<<endl;
		*/



		// get the root page
		string s = ixfileHandle.fileHandle.fileName + "_rootpage";
		FileHandle fileHandle_tmp;
		PagedFileManager::instance()->openFile(s, fileHandle_tmp);
		void* pageData_rootpage = malloc(PAGE_SIZE);
		fileHandle_tmp.readPage(0, pageData_rootpage);
		int rootpage;
		memcpy(&rootpage, (char*) pageData_rootpage, sizeof(int));
		ixfileHandle.rootpage = rootpage;
		PagedFileManager::instance()->closeFile(fileHandle_tmp);
		free(pageData_rootpage);


		/*
		void* pageDatatest = malloc(PAGE_SIZE);
		ixfileHandle.fileHandle.readPage(ixfileHandle.rootpage, pageDatatest);
		testnodeprint(pageDatatest);
		*/


		//read the record
		void* pageData = malloc(PAGE_SIZE);
		RC rc = ixfileHandle.fileHandle.readPage(ixfileHandle.rootpage, pageData);
		if(rc != 0)
			return -1;

		int IfLeaf;
		memcpy(&IfLeaf, ((char*) pageData) + 1 + 4 + 2 +1, sizeof(int));

		if (IfLeaf == 1){ // is leaf
			short NF[2];
			memcpy(NF, ((char*) pageData) + 4092, sizeof(short) * 2);


			if (NF[1] - KeyRidrecordSize - 4 > 100){ // still has the space for key
				// Apply binary search to find the place to insert the key
				BinarySearchInsert(ixfileHandle, KeyRidDescriptor, KeyRidrecord, NF[0],attribute, key, pageData, 0);
			}
			else{
				// need to split
				// find the middle point
				int end = 2;
				int begin = NF[0] -1;
				int mid = (begin + end)/2; // all key bigger than the new key will be moved to the new leaf page
				void *mid_key = malloc(4096);
				short mid_addr;
				memcpy(&mid_addr, ((char*) pageData) + 4096 - 4 - ((mid+1) * sizeof(short) * 2), sizeof(short));
				short mid_key_slot;
				memcpy(&mid_key_slot, ((char*) pageData) + mid_addr + 5, sizeof(short)); // 1 + 4
				memcpy(((char*)mid_key), ((char*) pageData) + mid_addr + 12, mid_key_slot - 12); // 1 + 4 + 2*3 +1

				// form the new leaf page
				//initialize a leaf
				vector<Attribute> IfLeafDescriptor;
				createIfLeafDescriptor(IfLeafDescriptor);
				int IfLeafnullFieldsIndicatorActualSize = ceil((double) IfLeafDescriptor.size() / CHAR_BIT);
				unsigned char *IfLeafnullsIndicator = (unsigned char *) malloc(IfLeafnullFieldsIndicatorActualSize);
				memset(IfLeafnullsIndicator, 0, IfLeafnullFieldsIndicatorActualSize);
				int IfLeafrecordSize = 0;
				void *IfLeafrecord = malloc(4096);
				prepareIfLeafRecord(IfLeafDescriptor.size(), IfLeafnullsIndicator, 1, IfLeafrecord, &IfLeafrecordSize); // the IfLeaf Indicator to indicate if it is a leaf or not
				free(IfLeafnullsIndicator);

				vector<Attribute> NextPagePointerDescriptor;
				createNextPagePointerDescriptor(NextPagePointerDescriptor);
				int NextPagePointernullFieldsIndicatorActualSize = ceil((double) NextPagePointerDescriptor.size() / CHAR_BIT);
				unsigned char *NextPagePointernullsIndicator = (unsigned char *) malloc(NextPagePointernullFieldsIndicatorActualSize);
				memset(NextPagePointernullsIndicator, 0, NextPagePointernullFieldsIndicatorActualSize);
				int NextPagePointerrecordSize = 0;
				void *NextPagePointerrecord = malloc(4096);
				prepareNextPagePointerRecord(NextPagePointerDescriptor.size(), NextPagePointernullsIndicator, -1, NextPagePointerrecord, &NextPagePointerrecordSize); // The page pointer to the next page initial be -1
				free(NextPagePointernullsIndicator);
				//append a new page then insert the record in this page
				ixfileHandle.fileHandle.appendPage2();
				int newpage = ixfileHandle.fileHandle.getNumberOfPages() - 1;

				RID rid;
				RecordBasedFileManager::instance()->IXinsertRecord(ixfileHandle.fileHandle, IfLeafDescriptor, IfLeafrecord, rid, newpage);
				RecordBasedFileManager::instance()->IXinsertRecord(ixfileHandle.fileHandle, NextPagePointerDescriptor, NextPagePointerrecord, rid, newpage);

				// for all elements bigger or equal to the mid point need to be insert in the new page
				int rest=NF[0] - mid; //the number of element that bigger or equal than mid
				int newrecord = mid;
				while (rest>0){
					// get the new record
					void *KeyRidrecord_tmp = malloc(4096); // the record that be reserved temperately
					short addr;
					memcpy(&addr, ((char*) pageData) + 4096 - 4 - ((newrecord+1) * sizeof(short) * 2), sizeof(short));
					short len;
					memcpy(&len, ((char*) pageData) + 4096 - 4 - ((newrecord+1) * sizeof(short) * 2) + 2, sizeof(short));
					memcpy(((char*) KeyRidrecord_tmp), ((char*) pageData) + addr + 11, len - 11); // 11 = 1 + 4 + 2 + 2 + 2 one indicator + offset + 3*slot
					//insert the new record to the new page
					RecordBasedFileManager::instance()->IXinsertRecord(ixfileHandle.fileHandle, KeyRidDescriptor, KeyRidrecord_tmp, rid, newpage);
					free(KeyRidrecord_tmp);
					newrecord++;
					rest=rest-1;
				}
				free(IfLeafrecord);
				free(NextPagePointerrecord);

				// update the NextPagePointerrecord in new page //
				void *NextPagePointerrecord_tmp = malloc(4096);
				short addr;
				memcpy(&addr, ((char*) pageData) + 4096 - 4 - (2 * sizeof(short) * 2), sizeof(short));
				short len;
				memcpy(&len, ((char*) pageData) + 4096 - 4 - (2 * sizeof(short) * 2) + 2, sizeof(short));
				memcpy(((char*) NextPagePointerrecord_tmp), ((char*) pageData) + addr + 7, len - 7); // 1 + 4 + 2
				rid.pageNum = newpage;
				rid.slotNum = 1;
				RecordBasedFileManager::instance()->updateRecord(ixfileHandle.fileHandle, NextPagePointerDescriptor, NextPagePointerrecord_tmp, rid);

				// update the NextPagePointerrecord in old page //
				memcpy(((char*)NextPagePointerrecord_tmp) + 1, &newpage, sizeof(int));
				rid.pageNum = ixfileHandle.rootpage;
				rid.slotNum = 1;
				RecordBasedFileManager::instance()->updateRecord(ixfileHandle.fileHandle, NextPagePointerDescriptor, NextPagePointerrecord_tmp, rid);

				free(NextPagePointerrecord_tmp);

				// update the N and F in old page as the deletion of the records bigger or equal mid //
				ixfileHandle.fileHandle.readPage(0, pageData);
				short NF_tmp[2];
				memcpy(NF_tmp, ((char*) pageData) + 4092, sizeof(short) * 2);
				NF_tmp[0] = mid;

				short addr_tmp;
				memcpy(&addr_tmp, ((char*) pageData) + 4096 - 4 - ((mid + 1) * sizeof(short) * 2), sizeof(short));
				NF_tmp[1] =4096 - 4 - 4*NF_tmp[0] - addr_tmp;

				memcpy(((char*) pageData) + 4092, NF_tmp, sizeof(short) * 2);

				short test_tmp[2];
				memcpy(test_tmp, ((char*) pageData) + 4092, sizeof(short) * 2);
				ixfileHandle.fileHandle.writePage(0, pageData);


				//insert the keyrid record//
				void* newpageData = malloc(PAGE_SIZE);
				RC rc = ixfileHandle.fileHandle.readPage(newpage, newpageData);
				short newNF[2];
				memcpy(newNF, ((char*) newpageData) + 4092, sizeof(short) * 2);
				if(key_nodekey_compare(attribute, key, mid_key) >= 0){ // return 0 if key = nodekey, return 1 if key > nodekey, return -1 if key < nodekey
					BinarySearchInsert(ixfileHandle, KeyRidDescriptor, KeyRidrecord, newNF[0],attribute, key, newpageData, newpage); //insert in new page
				}
				else{
					BinarySearchInsert(ixfileHandle, KeyRidDescriptor, KeyRidrecord, NF[0],attribute, key, pageData, 0); //insert in old page
				}
				free(newpageData);


				// initialize a root page //
				vector<Attribute> IfLeafDescriptor_node;
				createIfLeafDescriptor(IfLeafDescriptor_node);
				int IfLeafnullFieldsIndicatorActualSize_node = ceil((double) IfLeafDescriptor_node.size() / CHAR_BIT);
				unsigned char *IfLeafnullsIndicator_node = (unsigned char *) malloc(IfLeafnullFieldsIndicatorActualSize_node);
				memset(IfLeafnullsIndicator_node, 0, IfLeafnullFieldsIndicatorActualSize_node);
				int IfLeafrecordSize_node = 0;
				void *IfLeafrecord_node = malloc(4096);
				prepareIfLeafRecord(IfLeafDescriptor_node.size(), IfLeafnullsIndicator_node, 0, IfLeafrecord_node, &IfLeafrecordSize_node); // the IfLeaf Indicator to indicate if it is a leaf or not
				free(IfLeafnullsIndicator_node);

				vector<Attribute> FixedLeftPointerDescriptor;
				createFixedLeftPointerDescriptor(FixedLeftPointerDescriptor);
				int FixedLeftPointernullFieldsIndicatorActualSize = ceil((double) FixedLeftPointerDescriptor.size() / CHAR_BIT);
				unsigned char *FixedLeftPointernullsIndicator = (unsigned char *) malloc(FixedLeftPointernullFieldsIndicatorActualSize);
				memset(FixedLeftPointernullsIndicator, 0, FixedLeftPointernullFieldsIndicatorActualSize);
				int FixedLeftPointerrecordSize = 0;
				void *FixedLeftPointerrecord = malloc(4096);
				prepareFixedLeftPointerRecord(FixedLeftPointerDescriptor.size(), FixedLeftPointernullsIndicator, 0, FixedLeftPointerrecord, &FixedLeftPointerrecordSize); // the IfLeaf Indicator to indicate if it is a leaf or not
				free(FixedLeftPointernullsIndicator);

				vector<Attribute> KeyPointerDescriptor;
				KeyPointerDescriptor.push_back(attribute);
				createKeyPointerDescriptor(KeyPointerDescriptor);
				int KeyPointernullFieldsIndicatorActualSize = ceil((double) KeyPointerDescriptor.size() / CHAR_BIT);
				unsigned char *KeyPointernullsIndicator = (unsigned char *) malloc(KeyPointernullFieldsIndicatorActualSize);
				memset(KeyPointernullsIndicator, 0, KeyPointernullFieldsIndicatorActualSize);
				int KeyPointerrecordSize = 0;
				void *KeyPointerrecord = malloc(4096);
				prepareKeyPointerRecord(KeyPointerDescriptor.size(), KeyPointernullsIndicator, attribute, mid_key, newpage, KeyPointerrecord, &KeyPointerrecordSize);
				free(KeyPointernullsIndicator);

				//append a new page then insert the record in this page //
				ixfileHandle.fileHandle.appendPage2();
				int node_newpage = ixfileHandle.fileHandle.getNumberOfPages() - 1;

				//update the root page //
				PagedFileManager::instance()->openFile(s, fileHandle_tmp);
				void* pageData_rootpage2 = malloc(PAGE_SIZE);
				fileHandle_tmp.readPage(0, pageData_rootpage2);
				memcpy((char*)pageData_rootpage2, &node_newpage, sizeof(int));
				fileHandle_tmp.writePage(0,pageData_rootpage2);
				PagedFileManager::instance()->closeFile(fileHandle_tmp);
				free(pageData_rootpage2);

				RecordBasedFileManager::instance()->IXinsertRecord(ixfileHandle.fileHandle, IfLeafDescriptor, IfLeafrecord_node, rid, node_newpage);
				RecordBasedFileManager::instance()->IXinsertRecord(ixfileHandle.fileHandle, FixedLeftPointerDescriptor, FixedLeftPointerrecord, rid, node_newpage);
				RecordBasedFileManager::instance()->IXinsertRecord(ixfileHandle.fileHandle, KeyPointerDescriptor, KeyPointerrecord, rid, node_newpage);

				free(IfLeafrecord_node);
				free(FixedLeftPointerrecord);
				free(KeyPointerrecord);

				free(mid_key);
				free(pageData);
			}
		}
		else{
			// is node
			// introduce a stack to record the route from top to bottom
			stack <int> split;
			split.push(ixfileHandle.rootpage);

			int IfLeaf_tmp;
			memcpy(&IfLeaf_tmp, ((char*) pageData) + 1 + 4 + 2 +1, sizeof(int));

			while(IfLeaf_tmp == 0){
				// search for the page pointer
				short NF[2];
				memcpy(NF, ((char*) pageData) + 4092, sizeof(short) * 2);
				int rest = NF[0] - 2;
				int start = 2;
				int page;
				while(rest > 0){

					void *KeyPointerrecord_tmp = malloc(4096); // the record that be reserved temperately
					short addr;
					memcpy(&addr, ((char*) pageData) + 4096 - 4 - ((start+1) * sizeof(short) * 2), sizeof(short));
					short slot;
					memcpy(&slot, ((char*) pageData) + addr + 1 + 4, sizeof(short));
					memcpy(((char*) KeyPointerrecord_tmp), ((char*) pageData) + addr + 10, slot -10); // 1 + 4 + 2 + 2 + 1

					if(key_nodekey_compare(attribute, key, KeyPointerrecord_tmp) == 0){// return 0 if key = nodekey, return 1 if key > nodekey, return -1 if key < nodekey
						short slot_tmp;
						memcpy(&slot_tmp, ((char*) pageData) + addr + 1 + 4, sizeof(short));
						memcpy(&page, ((char*) pageData) + addr + slot_tmp, sizeof(int));
						break;
					}
					else if(key_nodekey_compare(attribute, key, KeyPointerrecord_tmp) == 1){
						if (start == NF[0] - 1){ // last key
							short slot_tmp;
							memcpy(&slot_tmp, ((char*) pageData) + addr + 1 + 4, sizeof(short));
							memcpy(&page, ((char*) pageData) + addr + slot_tmp, sizeof(int));
							break;
						}
					}
					else{
						if (start == 2){ // key is smaller than the first node_key
							memcpy(&page, ((char*) pageData) + addr - 4, sizeof(int));
							break;
						}
						else{
							short addr_tmp;
							memcpy(&addr_tmp, ((char*) pageData) + 4096 - 4 - ((start) * sizeof(short) * 2), sizeof(short));
							short slot_tmp;
							memcpy(&slot_tmp, ((char*) pageData) + addr_tmp + 1 + 4, sizeof(short));
							memcpy(&page, ((char*) pageData) + addr_tmp + slot_tmp, sizeof(int));
							break;
						}
					}
					free(KeyPointerrecord_tmp);

					start++;
					rest--;
				}
				// add the next page into the hash
				split.push(page);
				RC rc = ixfileHandle.fileHandle.readPage(page, pageData);
				memcpy(&IfLeaf_tmp, ((char*) pageData) + 1 + 4 + 2 +1, sizeof(int));
			}

			// arrive leaf
			int page =  split.top();
			split.pop();
			RC rc = ixfileHandle.fileHandle.readPage(page, pageData);

			 // is leaf
			short NF[2];
			memcpy(NF, ((char*) pageData) + 4092, sizeof(short) * 2);

			if (NF[1] - KeyRidrecordSize - 4 > 100){ // still has the space for key
				// Apply binary search to find the place to insert the key
				BinarySearchInsert(ixfileHandle, KeyRidDescriptor, KeyRidrecord, NF[0],attribute, key, pageData, page);
			}
			else{
				// need to split
				// find the middle point
				int end = 2;
				int begin = NF[0] -1;
				int mid = (begin + end)/2; // all key bigger than or equal to the new key will be moved to the new leaf page
				void *mid_key = malloc(4096);
				short mid_addr;
				memcpy(&mid_addr, ((char*) pageData) + 4096 - 4 - ((mid+1) * sizeof(short) * 2), sizeof(short));
				short mid_key_slot;
				memcpy(&mid_key_slot, ((char*) pageData) + mid_addr + 5, sizeof(short)); // 1 + 4
				memcpy(((char*)mid_key), ((char*) pageData) + mid_addr + 12, mid_key_slot - 12); // 1 + 4 + 2*3 +1

				// form the new leaf page
				//initialize a leaf
				vector<Attribute> IfLeafDescriptor;
				createIfLeafDescriptor(IfLeafDescriptor);
				int IfLeafnullFieldsIndicatorActualSize = ceil((double) IfLeafDescriptor.size() / CHAR_BIT);
				unsigned char *IfLeafnullsIndicator = (unsigned char *) malloc(IfLeafnullFieldsIndicatorActualSize);
				memset(IfLeafnullsIndicator, 0, IfLeafnullFieldsIndicatorActualSize);
				int IfLeafrecordSize = 0;
				void *IfLeafrecord = malloc(4096);
				prepareIfLeafRecord(IfLeafDescriptor.size(), IfLeafnullsIndicator, 1, IfLeafrecord, &IfLeafrecordSize); // the IfLeaf Indicator to indicate if it is a leaf or not
				free(IfLeafnullsIndicator);

				vector<Attribute> NextPagePointerDescriptor;
				createNextPagePointerDescriptor(NextPagePointerDescriptor);
				int NextPagePointernullFieldsIndicatorActualSize = ceil((double) NextPagePointerDescriptor.size() / CHAR_BIT);
				unsigned char *NextPagePointernullsIndicator = (unsigned char *) malloc(NextPagePointernullFieldsIndicatorActualSize);
				memset(NextPagePointernullsIndicator, 0, NextPagePointernullFieldsIndicatorActualSize);
				int NextPagePointerrecordSize = 0;
				void *NextPagePointerrecord = malloc(4096);
				prepareNextPagePointerRecord(NextPagePointerDescriptor.size(), NextPagePointernullsIndicator, -1, NextPagePointerrecord, &NextPagePointerrecordSize); // The page pointer to the next page initial be -1
				free(NextPagePointernullsIndicator);
				//append a new page then insert the record in this page
				ixfileHandle.fileHandle.appendPage2();
				int newpage = ixfileHandle.fileHandle.getNumberOfPages() - 1;

				RID rid;
				RecordBasedFileManager::instance()->IXinsertRecord(ixfileHandle.fileHandle, IfLeafDescriptor, IfLeafrecord, rid, newpage);
				RecordBasedFileManager::instance()->IXinsertRecord(ixfileHandle.fileHandle, NextPagePointerDescriptor, NextPagePointerrecord, rid, newpage);

				// for all elements bigger or equal to the mid point need to be insert in the new page
				int rest=NF[0] - mid; //the number of element that bigger or equal than mid
				int newrecord = mid;
				while (rest>0){
					// get the new record
					void *KeyRidrecord_tmp = malloc(4096); // the record that be reserved temperately
					short addr;
					memcpy(&addr, ((char*) pageData) + 4096 - 4 - ((newrecord+1) * sizeof(short) * 2), sizeof(short));
					short len;
					memcpy(&len, ((char*) pageData) + 4096 - 4 - ((newrecord+1) * sizeof(short) * 2) + 2, sizeof(short));
					memcpy(((char*) KeyRidrecord_tmp), ((char*) pageData) + addr + 11, len - 11); // 11 = 1 + 4 + 2 + 2 + 2 one indicator + offset + 3*slot

					short NF2[2];
					memcpy(NF2, ((char*) pageData) + 4092, sizeof(short) * 2);
					//insert the new record to the new page
					if(rest==3){
						int a=0;
						a=3;
					}
					RecordBasedFileManager::instance()->IXinsertRecord(ixfileHandle.fileHandle, KeyRidDescriptor, KeyRidrecord_tmp, rid, newpage);
					free(KeyRidrecord_tmp);
					newrecord++;
					rest=rest-1;
				}
				free(IfLeafrecord);
				free(NextPagePointerrecord);

				// update the NextPagePointerrecord in new page //
				void *NextPagePointerrecord_tmp = malloc(4096);
				short addr;
				memcpy(&addr, ((char*) pageData) + 4096 - 4 - (2 * sizeof(short) * 2), sizeof(short));
				short len;
				memcpy(&len, ((char*) pageData) + 4096 - 4 - (2 * sizeof(short) * 2) + 2, sizeof(short));
				memcpy(((char*) NextPagePointerrecord_tmp), ((char*) pageData) + addr + 7, len - 7); // 1 + 4 + 2
				rid.pageNum = newpage;
				rid.slotNum = 1;
				RecordBasedFileManager::instance()->updateRecord(ixfileHandle.fileHandle, NextPagePointerDescriptor, NextPagePointerrecord_tmp, rid);

				// update the NextPagePointerrecord in old page //
				memcpy(((char*)NextPagePointerrecord_tmp + 1), &newpage, sizeof(int));
				rid.pageNum = page;
				rid.slotNum = 1;
				RecordBasedFileManager::instance()->updateRecord(ixfileHandle.fileHandle, NextPagePointerDescriptor, NextPagePointerrecord_tmp, rid);
				free(NextPagePointerrecord_tmp);

				// update the N and F in old page as the deletion of the records bigger than mid //
				ixfileHandle.fileHandle.readPage(page, pageData);
				short NF_tmp[2];
				memcpy(NF_tmp, ((char*) pageData) + 4092, sizeof(short) * 2);
				NF_tmp[0] = mid;

				short addr_tmp;
				memcpy(&addr_tmp, ((char*) pageData) + 4096 - 4 - ((mid + 1) * sizeof(short) * 2), sizeof(short));
				NF_tmp[1] =4096 - 4 - 4*NF_tmp[0] - addr_tmp;

				memcpy(((char*) pageData) + 4092, NF_tmp, sizeof(short) * 2);

				short test_tmp[2];
				memcpy(test_tmp, ((char*) pageData) + 4092, sizeof(short) * 2);
				ixfileHandle.fileHandle.writePage(page, pageData);


				//insert the keyrid record//
				void* newpageData = malloc(PAGE_SIZE);
				RC rc = ixfileHandle.fileHandle.readPage(newpage, newpageData);
				short newNF[2];
				memcpy(newNF, ((char*) newpageData) + 4092, sizeof(short) * 2);
				if(key_nodekey_compare(attribute, key, mid_key) >= 0){ // return 0 if key = nodekey, return 1 if key > nodekey, return -1 if key < nodekey
					BinarySearchInsert(ixfileHandle, KeyRidDescriptor, KeyRidrecord, newNF[0],attribute, key, newpageData, newpage); //insert in new page
				}
				else{
					BinarySearchInsert(ixfileHandle, KeyRidDescriptor, KeyRidrecord, NF[0],attribute, key, pageData, page); //insert in old page
				}
				free(newpageData);

				while (!split.empty()){
					//visit the node
					page =  split.top();
					split.pop();
					RC rc = ixfileHandle.fileHandle.readPage(page, pageData);
					short NF[2];
					memcpy(NF, ((char*) pageData) + 4092, sizeof(short) * 2);

					// prepare the KeyPointerrecord
					vector<Attribute> KeyPointerDescriptor;
					KeyPointerDescriptor.push_back(attribute);
					createKeyPointerDescriptor(KeyPointerDescriptor);
					int KeyPointernullFieldsIndicatorActualSize = ceil((double) KeyPointerDescriptor.size() / CHAR_BIT);
					unsigned char *KeyPointernullsIndicator = (unsigned char *) malloc(KeyPointernullFieldsIndicatorActualSize);
					memset(KeyPointernullsIndicator, 0, KeyPointernullFieldsIndicatorActualSize);
					int KeyPointerrecordSize = 0;
					void *KeyPointerrecord = malloc(4096);
					prepareKeyPointerRecord(KeyPointerDescriptor.size(), KeyPointernullsIndicator, attribute, mid_key, newpage, KeyPointerrecord, &KeyPointerrecordSize);
					free(KeyPointernullsIndicator);

					if(NF[1] - KeyPointerrecordSize - 4 > 100){ // there is enough space in node for insert keypointer record
						int replace = find_the_replace_node(attribute, NF[0], mid_key, pageData);
						//insert the mid_key
						keynodeInsert(ixfileHandle, KeyPointerDescriptor, KeyPointerrecord, NF[0], attribute, mid_key, pageData, page, replace);
						break;
					}
					else{ // node page need to split

						// find the mid_key (update)
						mid = ((NF[0] - 1) + 2) / 2;
						memcpy(&mid_addr, ((char*) pageData) + 4096 - 4 - ((mid+1) * sizeof(short) * 2), sizeof(short));
						memcpy(&mid_key_slot, ((char*) pageData) + mid_addr + 5, sizeof(short)); // 1 + 4
						memcpy(((char*)mid_key), ((char*) pageData) + mid_addr + 10, mid_key_slot - 10); // 1 + 4 + 2*2 +1
						int mid_key_pointer;
						memcpy(&mid_key_pointer, ((char*) pageData) + mid_addr + mid_key_slot, sizeof(int));

						// update the newpage (initialize a new node page)
						vector<Attribute> IfLeafDescriptor_node;
						createIfLeafDescriptor(IfLeafDescriptor_node);
						int IfLeafnullFieldsIndicatorActualSize_node = ceil((double) IfLeafDescriptor_node.size() / CHAR_BIT);
						unsigned char *IfLeafnullsIndicator_node = (unsigned char *) malloc(IfLeafnullFieldsIndicatorActualSize_node);
						memset(IfLeafnullsIndicator_node, 0, IfLeafnullFieldsIndicatorActualSize_node);
						int IfLeafrecordSize_node = 0;
						void *IfLeafrecord_node = malloc(4096);
						prepareIfLeafRecord(IfLeafDescriptor_node.size(), IfLeafnullsIndicator_node, 0, IfLeafrecord_node, &IfLeafrecordSize_node); // the IfLeaf Indicator to indicate if it is a leaf or not
						free(IfLeafnullsIndicator_node);

						vector<Attribute> FixedLeftPointerDescriptor;
						createFixedLeftPointerDescriptor(FixedLeftPointerDescriptor);
						int FixedLeftPointernullFieldsIndicatorActualSize = ceil((double) FixedLeftPointerDescriptor.size() / CHAR_BIT);
						unsigned char *FixedLeftPointernullsIndicator = (unsigned char *) malloc(FixedLeftPointernullFieldsIndicatorActualSize);
						memset(FixedLeftPointernullsIndicator, 0, FixedLeftPointernullFieldsIndicatorActualSize);
						int FixedLeftPointerrecordSize = 0;
						void *FixedLeftPointerrecord = malloc(4096);
						prepareFixedLeftPointerRecord(FixedLeftPointerDescriptor.size(), FixedLeftPointernullsIndicator, mid_key_pointer, FixedLeftPointerrecord, &FixedLeftPointerrecordSize); // the IfLeaf Indicator to indicate if it is a leaf or not
						free(FixedLeftPointernullsIndicator);

						//append a new page then insert the record in this page //
						ixfileHandle.fileHandle.appendPage2();
						int node_newpage = ixfileHandle.fileHandle.getNumberOfPages() - 1;
						//update newpage
						newpage = node_newpage;

						RecordBasedFileManager::instance()->IXinsertRecord(ixfileHandle.fileHandle, IfLeafDescriptor, IfLeafrecord_node, rid, node_newpage);
						RecordBasedFileManager::instance()->IXinsertRecord(ixfileHandle.fileHandle, FixedLeftPointerDescriptor, FixedLeftPointerrecord, rid, node_newpage);

						// insert the key > mid_key in the new page
						rest = NF[0] - 1 - mid;
						newrecord = mid + 1;
						while (rest>0){
							// get the new record
							void *KeyPointerrecord_tmp = malloc(4096); // the record that be reserved temperately
							short addr;
							memcpy(&addr, ((char*) pageData) + 4096 - 4 - ((newrecord+1) * sizeof(short) * 2), sizeof(short));
							short len;
							memcpy(&len, ((char*) pageData) + 4096 - 4 - ((newrecord+1) * sizeof(short) * 2) + 2, sizeof(short));
							memcpy(((char*) KeyPointerrecord_tmp), ((char*) pageData) + addr + 9, len - 9); // 9 = 1 + 4 + 2 + 2 one indicator + offset + 2*slot
							//insert the new record to the new page
							RecordBasedFileManager::instance()->IXinsertRecord(ixfileHandle.fileHandle, KeyPointerDescriptor, KeyPointerrecord_tmp, rid, newpage);
							free(KeyPointerrecord_tmp);
							newrecord++;
							rest=rest-1;
						}

						free(IfLeafrecord_node);
						free(FixedLeftPointerrecord);

						// update the N F in old node page (delete the key >= the mid_key)
						ixfileHandle.fileHandle.readPage(page, pageData);
						memcpy(NF, ((char*) pageData) + 4092, sizeof(short) * 2);
						NF[0] = mid;
						memcpy(&addr_tmp, ((char*) pageData) + 4096 - 4 - ((mid + 1) * sizeof(short) * 2), sizeof(short));
						NF[1] =4096 - 4 - 4*NF[0] - addr_tmp;
						memcpy(((char*) pageData) + 4092, NF, sizeof(short) * 2);
						ixfileHandle.fileHandle.writePage(page, pageData);

						// insert the KeyPointerrecord in either new page or old page

						void* mid_key_tmp = malloc(4096);
						memcpy(((char*)mid_key_tmp), ((char*)KeyPointerrecord)+ 1, KeyPointerrecordSize - 1 - 4);

						void* newpageData2 = malloc(PAGE_SIZE);
						RC rc = ixfileHandle.fileHandle.readPage(newpage, newpageData2);
						memcpy(newNF, ((char*) newpageData2) + 4092, sizeof(short) * 2);

						if(key_nodekey_compare(attribute, mid_key_tmp, mid_key) >= 0){ // return 0 if key = nodekey, return 1 if key > nodekey, return -1 if key < nodekey
							int replace = find_the_replace_node(attribute, newNF[0], mid_key_tmp, newpageData2);
							keynodeInsert(ixfileHandle, KeyPointerDescriptor, KeyPointerrecord, newNF[0], attribute, mid_key_tmp, newpageData2, newpage, replace); //insert in new page
						}
						else{
							int replace = find_the_replace_node(attribute, NF[0], mid_key_tmp, pageData);
							keynodeInsert(ixfileHandle, KeyPointerDescriptor, KeyPointerrecord, NF[0], attribute, mid_key_tmp, pageData, page, replace); //insert in old page
						}
						free(newpageData2);
						free(mid_key_tmp);

						if(split.empty()){
							// initialize a upper node page //
							vector<Attribute> IfLeafDescriptor_node;
							createIfLeafDescriptor(IfLeafDescriptor_node);
							int IfLeafnullFieldsIndicatorActualSize_node = ceil((double) IfLeafDescriptor_node.size() / CHAR_BIT);
							unsigned char *IfLeafnullsIndicator_node = (unsigned char *) malloc(IfLeafnullFieldsIndicatorActualSize_node);
							memset(IfLeafnullsIndicator_node, 0, IfLeafnullFieldsIndicatorActualSize_node);
							int IfLeafrecordSize_node = 0;
							void *IfLeafrecord_node = malloc(4096);
							prepareIfLeafRecord(IfLeafDescriptor_node.size(), IfLeafnullsIndicator_node, 0, IfLeafrecord_node, &IfLeafrecordSize_node); // the IfLeaf Indicator to indicate if it is a leaf or not
							free(IfLeafnullsIndicator_node);

							vector<Attribute> FixedLeftPointerDescriptor;
							createFixedLeftPointerDescriptor(FixedLeftPointerDescriptor);
							int FixedLeftPointernullFieldsIndicatorActualSize = ceil((double) FixedLeftPointerDescriptor.size() / CHAR_BIT);
							unsigned char *FixedLeftPointernullsIndicator = (unsigned char *) malloc(FixedLeftPointernullFieldsIndicatorActualSize);
							memset(FixedLeftPointernullsIndicator, 0, FixedLeftPointernullFieldsIndicatorActualSize);
							int FixedLeftPointerrecordSize = 0;
							void *FixedLeftPointerrecord = malloc(4096);
							prepareFixedLeftPointerRecord(FixedLeftPointerDescriptor.size(), FixedLeftPointernullsIndicator, page, FixedLeftPointerrecord, &FixedLeftPointerrecordSize); // the IfLeaf Indicator to indicate if it is a leaf or not
							free(FixedLeftPointernullsIndicator);

							vector<Attribute> KeyPointerDescriptor;
							KeyPointerDescriptor.push_back(attribute);
							createKeyPointerDescriptor(KeyPointerDescriptor);
							int KeyPointernullFieldsIndicatorActualSize = ceil((double) KeyPointerDescriptor.size() / CHAR_BIT);
							unsigned char *KeyPointernullsIndicator = (unsigned char *) malloc(KeyPointernullFieldsIndicatorActualSize);
							memset(KeyPointernullsIndicator, 0, KeyPointernullFieldsIndicatorActualSize);
							int KeyPointerrecordSize = 0;
							void *KeyPointerrecord = malloc(4096);
							// mid_key has updated, and newpage has updated
							prepareKeyPointerRecord(KeyPointerDescriptor.size(), KeyPointernullsIndicator, attribute, mid_key, newpage, KeyPointerrecord, &KeyPointerrecordSize);
							free(KeyPointernullsIndicator);

							//append a new page then insert the record in this page //
							ixfileHandle.fileHandle.appendPage2();
							int node_newupperpage = ixfileHandle.fileHandle.getNumberOfPages() - 1;

							//update the root page //
							PagedFileManager::instance()->openFile(s, fileHandle_tmp);
							void* pageData_rootpage2 = malloc(PAGE_SIZE);
							fileHandle_tmp.readPage(0, pageData_rootpage2);
							memcpy((char*)pageData_rootpage2, &node_newupperpage, sizeof(int));
							fileHandle_tmp.writePage(0,pageData_rootpage2);
							PagedFileManager::instance()->closeFile(fileHandle_tmp);
							free(pageData_rootpage2);

							RecordBasedFileManager::instance()->IXinsertRecord(ixfileHandle.fileHandle, IfLeafDescriptor, IfLeafrecord_node, rid, node_newupperpage);
							RecordBasedFileManager::instance()->IXinsertRecord(ixfileHandle.fileHandle, FixedLeftPointerDescriptor, FixedLeftPointerrecord, rid, node_newupperpage);
							RecordBasedFileManager::instance()->IXinsertRecord(ixfileHandle.fileHandle, KeyPointerDescriptor, KeyPointerrecord, rid, node_newupperpage);

							free(IfLeafrecord_node);
							free(FixedLeftPointerrecord);
						}
						free(KeyPointerrecord);
					}
				}
				free(mid_key);
				free(pageData);
			}

		}
	}

	free (KeyRidrecord);
    return 0;
}

void readrid(IXFileHandle &ixfileHandle, RID &rid, void *pageData, int page, int slot){
	ixfileHandle.fileHandle.readPage(page, pageData);
	short addr;
	memcpy(&addr, ((char*) pageData) + 4096 - 4 - ((slot+1) * sizeof(short) * 2), sizeof(short));
	short start;
	memcpy(&start, ((char*) pageData) + addr + 1 + 4, sizeof(short));

	memcpy(&rid.pageNum, ((char*) pageData) + addr + start, sizeof(int));
	memcpy(&rid.slotNum, ((char*) pageData) + addr + start + 4, sizeof(int));
}

RC IndexManager::deleteEntry(IXFileHandle &ixfileHandle, const Attribute &attribute, const void *key, const RID &rid)
{
	// get the root page
	string s = ixfileHandle.fileHandle.fileName + "_rootpage";
	FileHandle fileHandle_tmp;
	PagedFileManager::instance()->openFile(s, fileHandle_tmp);
	void* pageData_rootpage = malloc(PAGE_SIZE);
	fileHandle_tmp.readPage(0, pageData_rootpage);
	int rootpage;
	memcpy(&rootpage, (char*) pageData_rootpage, sizeof(int));
	ixfileHandle.rootpage = rootpage;
	PagedFileManager::instance()->closeFile(fileHandle_tmp);
	free(pageData_rootpage);

	void* pageData = malloc(PAGE_SIZE);
	ixfileHandle.fileHandle.readPage(rootpage, pageData);

	int IfLeaf_tmp;
	memcpy(&IfLeaf_tmp, ((char*) pageData) + 1 + 4 + 2 +1, sizeof(int));
	int page = 0;
	while(IfLeaf_tmp == 0){
		// search for the page pointer
		short NF[2];
		memcpy(NF, ((char*) pageData) + 4092, sizeof(short) * 2);
		int rest = NF[0] - 2;
		int start = 2;
		while(rest > 0){

			void *KeyPointerrecord_tmp = malloc(4096); // the record that be reserved temperately
			short addr;
			memcpy(&addr, ((char*) pageData) + 4096 - 4 - ((start+1) * sizeof(short) * 2), sizeof(short));
			short slot;
			memcpy(&slot, ((char*) pageData) + addr + 1 + 4, sizeof(short));
			memcpy(((char*) KeyPointerrecord_tmp), ((char*) pageData) + addr + 10, slot -10); // 1 + 4 + 2 + 2 + 1

			if(key_nodekey_compare(attribute, key, KeyPointerrecord_tmp) == 0){// return 0 if key = nodekey, return 1 if key > nodekey, return -1 if key < nodekey
				short slot_tmp;
				memcpy(&slot_tmp, ((char*) pageData) + addr + 1 + 4, sizeof(short));
				memcpy(&page, ((char*) pageData) + addr + slot_tmp, sizeof(int));
				break;
			}
			else if(key_nodekey_compare(attribute, key, KeyPointerrecord_tmp) == 1){
				if (start == NF[0] - 1){ // last key
					short slot_tmp;
					memcpy(&slot_tmp, ((char*) pageData) + addr + 1 + 4, sizeof(short));
					memcpy(&page, ((char*) pageData) + addr + slot_tmp, sizeof(int));
					break;
				}
			}
			else{
				if (start == 2){ // key is smaller than the first node_key
					memcpy(&page, ((char*) pageData) + addr - 4, sizeof(int));
					break;
				}
				else{
					short addr_tmp;
					memcpy(&addr_tmp, ((char*) pageData) + 4096 - 4 - ((start) * sizeof(short) * 2), sizeof(short));
					short slot_tmp;
					memcpy(&slot_tmp, ((char*) pageData) + addr_tmp + 1 + 4, sizeof(short));
					memcpy(&page, ((char*) pageData) + addr_tmp + slot_tmp, sizeof(int));
					break;
				}
			}
			free(KeyPointerrecord_tmp);

			start++;
			rest--;
		}
		ixfileHandle.fileHandle.readPage(page, pageData);
		memcpy(&IfLeaf_tmp, ((char*) pageData) + 1 + 4 + 2 +1, sizeof(int));
	}
	short NF[2];
	memcpy(NF, ((char*) pageData) + 4092, sizeof(short) * 2);
	int rest = NF[0] - 2;
	int start = 2;
	while (rest > 0){
		short addr;
		memcpy(&addr, ((char*) pageData) + 4096 - 4 - ((start+1) * sizeof(short) * 2), sizeof(short));
		short slot;
		memcpy(&slot, ((char*) pageData) + addr + 1 + 4, sizeof(short));

		RID tmp;
		memcpy(&tmp.pageNum, ((char*) pageData) + addr + slot, sizeof(int));
		memcpy(&tmp.slotNum, ((char*) pageData) + addr + slot + 4, sizeof(int));

		if (compare(attribute, start, key, pageData) == 0 && tmp.pageNum == rid.pageNum && tmp.slotNum == rid.slotNum){ //find the equal key element
			// start delete
			RID del;
			del.pageNum = page;
			del.slotNum = start;

			vector<Attribute> KeyRidDescriptor;
			KeyRidDescriptor.push_back(attribute);
			createKeyRidDescriptor(KeyRidDescriptor);

			// delete the record
			RecordBasedFileManager::instance()->deleteRecord(ixfileHandle.fileHandle, KeyRidDescriptor, del);

			//move the slot in page;
			ixfileHandle.fileHandle.readPage(page, pageData);
			int last = NF[0] - start - 1;
			if (last>0){
				memmove((char*) pageData + 4092 - NF[0]*4 + 4 , (char*) pageData + 4092 - NF[0]*4, last*2*sizeof(short));
				//memcpy((char*) pageData + 4092 - start*4 , (char*) pageData + 4092 - start*4 - 4, last*2*sizeof(short));
			}
			//update N and F
			short NF_tmp[2];
			memcpy(NF_tmp, ((char*) pageData) + 4092, sizeof(short) * 2);
			NF_tmp[0]--;
			NF_tmp[1] = NF_tmp[1] + 4;

			memcpy(((char*) pageData) + 4092, NF_tmp, sizeof(short) * 2);

			short test_tmp[2];
			memcpy(test_tmp, ((char*) pageData) + 4092, sizeof(short) * 2);


			ixfileHandle.fileHandle.writePage(page, pageData);
			free(pageData);
			return 0;
		}
		rest--;
		start++;
	}
	free(pageData);
    return -1;
}


RC IndexManager::scan(IXFileHandle &ixfileHandle,
        const Attribute &attribute,
        const void      *lowKey,
        const void      *highKey,
        bool			lowKeyInclusive,
        bool        	highKeyInclusive,
        IX_ScanIterator &ix_ScanIterator)
{
	IXFileHandle tmp = *(&ixfileHandle);
//	IXFileHandle tmp = ixfileHandle;
	ix_ScanIterator.ixfileHandle = tmp;
	ix_ScanIterator.attribute = attribute;
	ix_ScanIterator.lowKey=lowKey;
	ix_ScanIterator.highKey=highKey;
	ix_ScanIterator.lowKeyInclusive=lowKeyInclusive;
	ix_ScanIterator.highKeyInclusive=highKeyInclusive;

	if (!ixfileHandle.fileHandle.file){
		return -1;
	}

	if (!lowKey){ //lowkey = NULL
		// start from page 0 slot 2;
		ix_ScanIterator.page = 0;
		ix_ScanIterator.slot = 2;
		void* pageData = malloc(PAGE_SIZE);
		ixfileHandle.fileHandle.readPage(0, pageData);
		short NF_tmp[2];
		memcpy(NF_tmp, ((char*) pageData) + 4092, sizeof(short) * 2);
		ix_ScanIterator.N = NF_tmp[0];
		free(pageData);
	}
	else{
		// get the root page
		string s = ixfileHandle.fileHandle.fileName + "_rootpage";
		FileHandle fileHandle_tmp;
		PagedFileManager::instance()->openFile(s, fileHandle_tmp);
		void* pageData_rootpage = malloc(PAGE_SIZE);
		fileHandle_tmp.readPage(0, pageData_rootpage);
		int rootpage;
		memcpy(&rootpage, (char*) pageData_rootpage, sizeof(int));
		ixfileHandle.rootpage = rootpage;
		PagedFileManager::instance()->closeFile(fileHandle_tmp);
		free(pageData_rootpage);

		void* pageData = malloc(PAGE_SIZE);
		ixfileHandle.fileHandle.readPage(rootpage, pageData);

		int IfLeaf_tmp;
		memcpy(&IfLeaf_tmp, ((char*) pageData) + 1 + 4 + 2 +1, sizeof(int));
		int page = 0;
		while(IfLeaf_tmp == 0){
			// search for the page pointer
			short NF[2];
			memcpy(NF, ((char*) pageData) + 4092, sizeof(short) * 2);
			int rest = NF[0] - 2;
			int start = 2;
			while(rest > 0){

				void *KeyPointerrecord_tmp = malloc(4096); // the record that be reserved temperately
				short addr;
				memcpy(&addr, ((char*) pageData) + 4096 - 4 - ((start+1) * sizeof(short) * 2), sizeof(short));
				short slot;
				memcpy(&slot, ((char*) pageData) + addr + 1 + 4, sizeof(short));
				memcpy(((char*) KeyPointerrecord_tmp), ((char*) pageData) + addr + 10, slot -10); // 1 + 4 + 2 + 2 + 1

				if(key_nodekey_compare(attribute, lowKey, KeyPointerrecord_tmp) == 0){// return 0 if key = nodekey, return 1 if key > nodekey, return -1 if key < nodekey
					short slot_tmp;
					memcpy(&slot_tmp, ((char*) pageData) + addr + 1 + 4, sizeof(short));
					memcpy(&page, ((char*) pageData) + addr + slot_tmp, sizeof(int));
					break;
				}
				else if(key_nodekey_compare(attribute, lowKey, KeyPointerrecord_tmp) == 1){
					if (start == NF[0] - 1){ // last key
						short slot_tmp;
						memcpy(&slot_tmp, ((char*) pageData) + addr + 1 + 4, sizeof(short));
						memcpy(&page, ((char*) pageData) + addr + slot_tmp, sizeof(int));
						break;
					}
				}
				else{
					if (start == 2){ // key is smaller than the first node_key
						memcpy(&page, ((char*) pageData) + addr - 4, sizeof(int));
						break;
					}
					else{
						short addr_tmp;
						memcpy(&addr_tmp, ((char*) pageData) + 4096 - 4 - ((start) * sizeof(short) * 2), sizeof(short));
						short slot_tmp;
						memcpy(&slot_tmp, ((char*) pageData) + addr_tmp + 1 + 4, sizeof(short));
						memcpy(&page, ((char*) pageData) + addr_tmp + slot_tmp, sizeof(int));
						break;
					}
				}
				free(KeyPointerrecord_tmp);

				start++;
				rest--;
			}
			ixfileHandle.fileHandle.readPage(page, pageData);
			memcpy(&IfLeaf_tmp, ((char*) pageData) + 1 + 4 + 2 +1, sizeof(int));
		}
		ix_ScanIterator.page = page;
		
		if (lowKeyInclusive){ // find the first slot in page >= lowKey
			short NF[2];
			memcpy(NF, ((char*) pageData) + 4092, sizeof(short) * 2);
			int next;
			memcpy(&next, ((char*) pageData) + 12 + 8, sizeof(int));
			if(NF[0] != 2){
				int rest = NF[0] - 2;
				int start = 2;
				while(rest>0){
					if(compare(attribute, start, lowKey, pageData) <= 0){ //lowkey < start slot
						ix_ScanIterator.page = page;
						ix_ScanIterator.slot = start;
						break;
					}
					rest--;
					start++;
				}
				if(rest == 0){ // no element found in this page
					if(next == -1){ //empty page and no next page
						ix_ScanIterator.page = -1;
						ix_ScanIterator.slot = 2;
					}
					int final=0;
					while (next != -1){ // find the right page
						int rest = NF[0] - 2;
						int start = 2;
						while(rest>0){
							if(compare(attribute, start, lowKey, pageData) <= 0){ //lowkey < start slot
								ix_ScanIterator.page = page;
								ix_ScanIterator.slot = start;
								final=1;
								break;
							}
							rest--;
							start++;
						}
						if(rest == 0){ // no element found in this page (go to next page)
							ixfileHandle.fileHandle.readPage(next, pageData);
							memcpy(&next, ((char*) pageData) + 12 + 8, sizeof(int));
							memcpy(NF, ((char*) pageData) + 4092, sizeof(short) * 2);
						}
					}
					if (final==0){
						ix_ScanIterator.page = -1; // non element found
						ix_ScanIterator.slot = 2;
					}
				}
			}
			else{
				if(next == -1){ //empty page and no next page
					ix_ScanIterator.page = page;
					ix_ScanIterator.slot = 2;
				}
				int final=0;
				while (next != -1){ // find the right page
					int rest = NF[0] - 2;
					int start = 2;
					while(rest>0){
						if(compare(attribute, start, lowKey, pageData) <= 0){ //lowkey < start slot
							ix_ScanIterator.page = page;
							ix_ScanIterator.slot = start;
							final=1;
							break;
						}
						rest--;
						start++;
					}
					if(rest == 0){ // no element found in this page (go to next page)
						ixfileHandle.fileHandle.readPage(next, pageData);
						memcpy(&next, ((char*) pageData) + 12 + 8, sizeof(int));
						memcpy(NF, ((char*) pageData) + 4092, sizeof(short) * 2);
					}
				}
				if (final==0){
					ix_ScanIterator.page = -1; // non element found
					ix_ScanIterator.slot = 2;
				}
			}
		}
		else{ // find the first slot in page > lowKey
			short NF[2];
			memcpy(NF, ((char*) pageData) + 4092, sizeof(short) * 2);
			int next;
			memcpy(&next, ((char*) pageData) + 12 + 8, sizeof(int));
			if(NF[0] != 2){
				int rest = NF[0] - 2;
				int start = 2;
				while(rest>0){
					if(compare(attribute, start, lowKey, pageData) < 0){ //lowkey < start slot
						ix_ScanIterator.page = page;
						ix_ScanIterator.slot = start;
						break;
					}
					rest--;
					start++;
				}
				if(rest == 0){ // no element found in this page
					if(next == -1){ //empty page and no next page
						ix_ScanIterator.page = -1;
						ix_ScanIterator.slot = 2;
					}
					int final=0;
					while (next != -1){ // find the right page
						int rest = NF[0] - 2;
						int start = 2;
						while(rest>0){
							if(compare(attribute, start, lowKey, pageData) < 0){ //lowkey < start slot
								ix_ScanIterator.page = page;
								ix_ScanIterator.slot = start;
								final=1;
								break;
							}
							rest--;
							start++;
						}
						if(rest == 0){ // no element found in this page (go to next page)
							ixfileHandle.fileHandle.readPage(next, pageData);
							memcpy(&next, ((char*) pageData) + 12 + 8, sizeof(int));
							memcpy(NF, ((char*) pageData) + 4092, sizeof(short) * 2);
						}
					}
					if (final==0){
						ix_ScanIterator.page = -1; // non element found
						ix_ScanIterator.slot = 2;
					}
				}
			}
			else{
				if(next == -1){ //empty page and no next page
					ix_ScanIterator.page = page;
					ix_ScanIterator.slot = 2;
				}
				int final=0;
				while (next != -1){ // find the right page
					int rest = NF[0] - 2;
					int start = 2;
					while(rest>0){
						if(compare(attribute, start, lowKey, pageData) < 0){ //lowkey < start slot
							ix_ScanIterator.page = page;
							ix_ScanIterator.slot = start;
							final=1;
							break;
						}
						rest--;
						start++;
					}
					if(rest == 0){ // no element found in this page (go to next page)
						ixfileHandle.fileHandle.readPage(next, pageData);
						memcpy(&next, ((char*) pageData) + 12 + 8, sizeof(int));
						memcpy(NF, ((char*) pageData) + 4092, sizeof(short) * 2);
					}
				}
				if (final==0){
					ix_ScanIterator.page = -1; // non element found
					ix_ScanIterator.slot = 2;
				}
			}
		}
		short NF_tmp[2];
		memcpy(NF_tmp, ((char*) pageData) + 4092, sizeof(short) * 2);
		ix_ScanIterator.N=NF_tmp[0];
		free(pageData);
	}
    return 0;
}

void IndexManager::printBtree(IXFileHandle &ixfileHandle, const Attribute &attribute) {
    void* pageData = malloc(PAGE_SIZE);

    // get the root page
    string s = ixfileHandle.fileHandle.fileName + "_rootpage";
    FileHandle fileHandle_tmp;
    PagedFileManager::instance()->openFile(s, fileHandle_tmp);
    void* pageData_rootpage = malloc(PAGE_SIZE);
    fileHandle_tmp.readPage(0, pageData_rootpage);
    int rootPage;
    memcpy(&rootPage, (char*) pageData_rootpage, sizeof(int));
    ixfileHandle.rootpage = rootPage;
    PagedFileManager::instance()->closeFile(fileHandle_tmp);
    free(pageData_rootpage);


    //read rootPage (here is only for not letting pageData become 0 byte)
//  int rootPage = ixfileHandle.rootpage;
//    cout << "rootPage:" << rootPage << endl;
    ixfileHandle.fileHandle.readPage(rootPage, pageData);

    //start to print
    printBtreeHelper(ixfileHandle, attribute, 0, rootPage, pageData);
    free(pageData);
}

bool checkLeaf(void* pageData){
    int isLeaf = 0;

    // set isLeaf
    // indicator(1byte) + fieldCount(4bytes) + offset(2bytes) + null_indicator(1byte)
    memcpy(&isLeaf, ((char*)pageData) + 1 + 4 + 2 + 1, sizeof(int));

    if(isLeaf == 1)
        return true;
    else
        return false;
}

void printKeyAndRID(void* recordData, const Attribute &attribute){
    int offset = 0;
//  int offset = slotInfo[0];
    int type = attribute.type;

    cout << "\"";

    // indicator(1byte) + fieldCount(4bytes) + offset(2 + 2 + 2 bytes) + null_indicator (1byte)
    offset += 1 + 4 + 6 + 1;

    switch(type){
        case TypeInt:
            int key_int;
            memcpy(&key_int, ((char*)recordData) + offset, sizeof(int));

            cout << key_int;
            offset += sizeof(int);
            break;
        case TypeReal:
            float key_float;
            memcpy(&key_float, ((char*)recordData) + offset, sizeof(float));

            cout << key_float;
            offset += sizeof(float);
            break;
        case TypeVarChar:
            int length;
            memcpy(&length, ((char*)recordData) + offset, sizeof(int));
            offset += sizeof(int);

            void* varChar = malloc(length);
            memcpy(varChar, ((char*)recordData) + offset, length);
            cout << *((char*)varChar);
            free(varChar);

            offset += length;
            break;
    }

    //key finish, then RID
    cout << ":[";

    int page;
    int slot;
    //set page
    memcpy(&page, ((char*)recordData) + offset, sizeof(int));
    offset += sizeof(int);
    //set slot
    memcpy(&slot, ((char*)recordData) + offset, sizeof(int));
    offset += sizeof(int);
    //cout << "offset:" << offset << endl;

    cout << "(" << page << "," << slot << ")";
    cout << "]\"";
}

void processLeftMostPagePointer(void* recordData, vector<int> &pageVector){

    // indicator(1byte) + fieldCount(4bytes) + offset(2bytes) + null_indicator (1byte)
    int offset = 1 + 4 + 2 + 1;
//  int offset = slotInfo[0] + 1 + 4 + 2 + 1;
//
//  cout << endl << "offset:" << offset << endl;

    int pageNum;
    memcpy(&pageNum, ((char*)recordData) + offset, sizeof(int));
//  cout << "page:" << pageNum << endl;
    pageVector.push_back(pageNum);
}

void processKeyWithRightPagePointer(void* recordData, vector<int> &pageVector, const Attribute &attribute){
    // indicator(1byte) + fieldCount(4bytes) + offset(2+2bytes) + null_indicator (1byte)
    int offset = 1 + 4 + 4 + 1;
    int type = attribute.type;

    switch(type){
        case TypeInt:
            int key_int;
            memcpy(&key_int, ((char*)recordData) + offset, sizeof(int));

            cout << key_int;
            offset += sizeof(int);
            break;
        case TypeReal:
            float key_float;
            memcpy(&key_float, ((char*)recordData) + offset, sizeof(float));

            cout << key_float;
            offset += sizeof(float);
            break;
        case TypeVarChar:
            int length;
            memcpy(&length, ((char*)recordData) + offset, sizeof(int));
            offset += sizeof(int);

//          char* keyVarChar;
//          memcpy(keyVarChar, ((char*)recordData) + offset,length);
//          cout << keyVarChar;
            void* varChar = malloc(length);
            memcpy(varChar, ((char*)recordData) + offset, length);
            cout << *((char*)varChar);
            free(varChar);


            offset +=  length;
            break;
    }

    int page;
    memcpy(&page, ((char*)recordData) + offset, sizeof(int));
//  cout << "page:" << page << endl;
    offset += sizeof(int);
    pageVector.push_back(page);
}

void IndexManager::printBtreeHelper(IXFileHandle &ixfileHandle, const Attribute &attribute, int level, int pageNum, void* pageData) {

    // read pageData
    ixfileHandle.fileHandle.readPage(pageNum, pageData);

    // read isLeaf (can be fixed because all are the same)
    bool isLeaf = checkLeaf(pageData);

    // get number of slots in page
    short N;
    memcpy(&N, ((char*)pageData) + 4092, sizeof(short));

    // indent
    for(int i = 0; i < level; i++){
        cout << "\t";
    }

    if(isLeaf){
        cout << "{\"keys\": [";

        //iterate all leaf nodes
        for(int i = 2; i < N; i++){
            //print ',' except the first one
            if(i != 2)
                cout << ",";

            short slotInfo[2];
            memcpy(slotInfo, ((char*)pageData) + 4092 - ((i+1) * 4), sizeof(short) * 2);

//          cout << endl;
//          cout << "i:" << i << endl;
//          cout << "N:" << N << endl;
//          cout << 4092 - ((i+1) * 4) << endl;
//          cout << slotInfo[0] << "   " << slotInfo[1] << endl;
            cout << endl;
            void* recordData = malloc(slotInfo[1]);
            memcpy(recordData, ((char*)pageData) + slotInfo[0], slotInfo[1]);
            printKeyAndRID(recordData, attribute);

            free(recordData);
        }
        //end all <key,RID> pair
        cout << "]}\n";

    }else{  //not leaf
        cout << "{\"keys\":[";

        // store next print
        vector<int> pageVector;
        for(int i = 1; i < N; i++){
            if(i > 2) //1:left_fixed   2: first display
                cout << ",";

            short slotInfo[2];
            memcpy(slotInfo, ((char*)pageData) + 4092 - ((i+1) * 4), sizeof(short) * 2);

            void* recordData = malloc(slotInfo[1]);
            memcpy(recordData, ((char*)pageData) + slotInfo[0], slotInfo[1]);

            //deal with fixed leftmost page pointer
            if(i == 1){
                processLeftMostPagePointer(recordData, pageVector);
            }else{ // normal <key, pageNum> pair
                processKeyWithRightPagePointer(recordData, pageVector, attribute);
            }
        }
        cout << "],\n";

        // start to traverse all children
        for(int i=0; i < level; i++)
            cout << "\t";
        cout << "\"children\":[" << endl;

        for(int i = 0; i < pageVector.size(); i++){
            printBtreeHelper(ixfileHandle, attribute, level + 1, pageVector[i], pageData);
//          if(i != 0)
//              cout << ",";
        }
        for(int i=0; i < level; i++)
            cout << "\t";
        cout << "]}\n";

    }

}


IX_ScanIterator::IX_ScanIterator()
{
}

IX_ScanIterator::~IX_ScanIterator()
{
}

void readkey(IXFileHandle &ixfileHandle, Attribute &attribute, RID &rid, void *pageData, int page, int slot, void *key){
	short addr;
	memcpy(&addr, ((char*) pageData) + 4096 - 4 - ((slot+1) * sizeof(short) * 2), sizeof(short));
	short place;
	memcpy(&place, ((char*) pageData) + addr + 1 + 4, sizeof(short));

	if(attribute.type == TypeReal || attribute.type == TypeInt){
		memcpy((char*)key, ((char*) pageData) + addr + 1 + 4 + 2*3 +1, place-12);
	}
	else{
		int length;
		memcpy(&length, ((char*) pageData) + addr + 1 + 4 + 2*3 + 1, sizeof(int));
		memcpy((char*)key, ((char*) pageData) + addr + 1 + 4 + 2*3 +1, length + 4);
	}
}


void scanhelper(IXFileHandle &ixfileHandle,
        const Attribute &attribute,
        const void      *lowKey,
        bool			lowKeyInclusive,
        IX_ScanIterator &ix_ScanIterator)
{
	if (!lowKey){ //lowkey = NULL
		// start from page 0 slot 2;
		ix_ScanIterator.page = 0;
		ix_ScanIterator.slot = 2;
	}
	else{
		// get the root page
		string s = ixfileHandle.fileHandle.fileName + "_rootpage";
		FileHandle fileHandle_tmp;
		PagedFileManager::instance()->openFile(s, fileHandle_tmp);
		void* pageData_rootpage = malloc(PAGE_SIZE);
		fileHandle_tmp.readPage(0, pageData_rootpage);
		int rootpage;
		memcpy(&rootpage, (char*) pageData_rootpage, sizeof(int));
		ixfileHandle.rootpage = rootpage;
		PagedFileManager::instance()->closeFile(fileHandle_tmp);
		free(pageData_rootpage);

		void* pageData = malloc(PAGE_SIZE);
		ixfileHandle.fileHandle.readPage(rootpage, pageData);

		int IfLeaf_tmp;
		memcpy(&IfLeaf_tmp, ((char*) pageData) + 1 + 4 + 2 +1, sizeof(int));
		int page = 0;
		while(IfLeaf_tmp == 0){
			// search for the page pointer
			short NF[2];
			memcpy(NF, ((char*) pageData) + 4092, sizeof(short) * 2);
			int rest = NF[0] - 2;
			int start = 2;
			while(rest > 0){

				void *KeyPointerrecord_tmp = malloc(4096); // the record that be reserved temperately
				short addr;
				memcpy(&addr, ((char*) pageData) + 4096 - 4 - ((start+1) * sizeof(short) * 2), sizeof(short));
				short slot;
				memcpy(&slot, ((char*) pageData) + addr + 1 + 4, sizeof(short));
				memcpy(((char*) KeyPointerrecord_tmp), ((char*) pageData) + addr + 10, slot -10); // 1 + 4 + 2 + 2 + 1

				if(key_nodekey_compare(attribute, lowKey, KeyPointerrecord_tmp) == 0){// return 0 if key = nodekey, return 1 if key > nodekey, return -1 if key < nodekey
					short slot_tmp;
					memcpy(&slot_tmp, ((char*) pageData) + addr + 1 + 4, sizeof(short));
					memcpy(&page, ((char*) pageData) + addr + slot_tmp, sizeof(int));
					break;
				}
				else if(key_nodekey_compare(attribute, lowKey, KeyPointerrecord_tmp) == 1){
					if (start == NF[0] - 1){ // last key
						short slot_tmp;
						memcpy(&slot_tmp, ((char*) pageData) + addr + 1 + 4, sizeof(short));
						memcpy(&page, ((char*) pageData) + addr + slot_tmp, sizeof(int));
						break;
					}
				}
				else{
					if (start == 2){ // key is smaller than the first node_key
						memcpy(&page, ((char*) pageData) + addr - 4, sizeof(int));
						break;
					}
					else{
						short addr_tmp;
						memcpy(&addr_tmp, ((char*) pageData) + 4096 - 4 - ((start) * sizeof(short) * 2), sizeof(short));
						short slot_tmp;
						memcpy(&slot_tmp, ((char*) pageData) + addr_tmp + 1 + 4, sizeof(short));
						memcpy(&page, ((char*) pageData) + addr_tmp + slot_tmp, sizeof(int));
						break;
					}
				}
				free(KeyPointerrecord_tmp);

				start++;
				rest--;
			}
			ixfileHandle.fileHandle.readPage(page, pageData);
			memcpy(&IfLeaf_tmp, ((char*) pageData) + 1 + 4 + 2 +1, sizeof(int));
		}
		ix_ScanIterator.page = page;

		if (lowKeyInclusive){ // find the first slot in page >= lowKey
			short NF[2];
			memcpy(NF, ((char*) pageData) + 4092, sizeof(short) * 2);
			int next;
			memcpy(&next, ((char*) pageData) + 12 + 8, sizeof(int));
			if(NF[0] != 2){
				int rest = NF[0] - 2;
				int start = 2;
				while(rest>0){
					if(compare(attribute, start, lowKey, pageData) <= 0){ //lowkey < start slot
						ix_ScanIterator.page = page;
						ix_ScanIterator.slot = start;
						break;
					}
					rest--;
					start++;
				}
				if(rest == 0){ // no element found in this page
					if(next == -1){ //empty page and no next page
						ix_ScanIterator.page = -1;
						ix_ScanIterator.slot = 2;
					}
					int final=0;
					while (next != -1){ // find the right page
						int rest = NF[0] - 2;
						int start = 2;
						while(rest>0){
							if(compare(attribute, start, lowKey, pageData) <= 0){ //lowkey < start slot
								ix_ScanIterator.page = page;
								ix_ScanIterator.slot = start;
								final=1;
								break;
							}
							rest--;
							start++;
						}
						if(rest == 0){ // no element found in this page (go to next page)
							ixfileHandle.fileHandle.readPage(next, pageData);
							memcpy(&next, ((char*) pageData) + 12 + 8, sizeof(int));
							memcpy(NF, ((char*) pageData) + 4092, sizeof(short) * 2);
						}
					}
					if (final==0){
						ix_ScanIterator.page = -1; // non element found
						ix_ScanIterator.slot = 2;
					}
				}
			}
			else{
				if(next == -1){ //empty page and no next page
					ix_ScanIterator.page = page;
					ix_ScanIterator.slot = 2;
				}
				int final=0;
				while (next != -1){ // find the right page
					int rest = NF[0] - 2;
					int start = 2;
					while(rest>0){
						if(compare(attribute, start, lowKey, pageData) <= 0){ //lowkey < start slot
							ix_ScanIterator.page = page;
							ix_ScanIterator.slot = start;
							final=1;
							break;
						}
						rest--;
						start++;
					}
					if(rest == 0){ // no element found in this page (go to next page)
						ixfileHandle.fileHandle.readPage(next, pageData);
						memcpy(&next, ((char*) pageData) + 12 + 8, sizeof(int));
						memcpy(NF, ((char*) pageData) + 4092, sizeof(short) * 2);
					}
				}
				if (final==0){
					ix_ScanIterator.page = -1; // non element found
					ix_ScanIterator.slot = 2;
				}
			}
		}
		else{ // find the first slot in page > lowKey
			short NF[2];
			memcpy(NF, ((char*) pageData) + 4092, sizeof(short) * 2);
			int next;
			memcpy(&next, ((char*) pageData) + 12 + 8, sizeof(int));
			if(NF[0] != 2){
				int rest = NF[0] - 2;
				int start = 2;
				while(rest>0){
					if(compare(attribute, start, lowKey, pageData) < 0){ //lowkey < start slot
						ix_ScanIterator.page = page;
						ix_ScanIterator.slot = start;
						break;
					}
					rest--;
					start++;
				}
				if(rest == 0){ // no element found in this page
					if(next == -1){ //empty page and no next page
						ix_ScanIterator.page = -1;
						ix_ScanIterator.slot = 2;
					}
					int final=0;
					while (next != -1){ // find the right page
						int rest = NF[0] - 2;
						int start = 2;
						while(rest>0){
							if(compare(attribute, start, lowKey, pageData) < 0){ //lowkey < start slot
								ix_ScanIterator.page = page;
								ix_ScanIterator.slot = start;
								final=1;
								break;
							}
							rest--;
							start++;
						}
						if(rest == 0){ // no element found in this page (go to next page)
							ixfileHandle.fileHandle.readPage(next, pageData);
							memcpy(&next, ((char*) pageData) + 12 + 8, sizeof(int));
							memcpy(NF, ((char*) pageData) + 4092, sizeof(short) * 2);
						}
					}
					if (final==0){
						ix_ScanIterator.page = -1; // non element found
						ix_ScanIterator.slot = 2;
					}
				}
			}
			else{
				if(next == -1){ //empty page and no next page
					ix_ScanIterator.page = page;
					ix_ScanIterator.slot = 2;
				}
				int final=0;
				while (next != -1){ // find the right page
					int rest = NF[0] - 2;
					int start = 2;
					while(rest>0){
						if(compare(attribute, start, lowKey, pageData) < 0){ //lowkey < start slot
							ix_ScanIterator.page = page;
							ix_ScanIterator.slot = start;
							final=1;
							break;
						}
						rest--;
						start++;
					}
					if(rest == 0){ // no element found in this page (go to next page)
						ixfileHandle.fileHandle.readPage(next, pageData);
						memcpy(&next, ((char*) pageData) + 12 + 8, sizeof(int));
						memcpy(NF, ((char*) pageData) + 4092, sizeof(short) * 2);
					}
				}
				if (final==0){
					ix_ScanIterator.page = -1; // non element found
					ix_ScanIterator.slot = 2;
				}
			}
		}
		free(pageData);
	}
}

RC IX_ScanIterator::getNextEntry(RID &rid, void *key)
{
	if(page == -1){
		return -1;
	}

	void* pageData_tmp = malloc(PAGE_SIZE);
	ixfileHandle.fileHandle.readPage(page, pageData_tmp);
	short NF_tmp[2];
	memcpy(NF_tmp, ((char*) pageData_tmp) + 4092, sizeof(short) * 2);
	if(NF_tmp[0] != N){
		N = NF_tmp[0];
		slot--;
	}
	free(pageData_tmp);

	if (!highKey){ //highKey = NULL
		// scan all
		void* pageData = malloc(PAGE_SIZE);
		ixfileHandle.fileHandle.readPage(page, pageData);
		int next;
		memcpy(&next, ((char*) pageData) + 12 + 8, sizeof(int));
		short NF[2];
		memcpy(NF, ((char*) pageData) + 4092, sizeof(short) * 2);

		if(slot != NF[0]){
			//read the rid page and slot
			readrid(ixfileHandle, rid, pageData, page, slot);
			readkey(ixfileHandle, attribute, rid, pageData, page, slot, key);
			int t;
			memcpy(&t, ((char*) key), 4);
			//page is same
			slot++;
		}
		else{
			while(slot == NF[0]){
				if(next == -1){
					return -1;
				}
				else{
					ixfileHandle.fileHandle.readPage(next, pageData);
					memcpy(NF, ((char*) pageData) + 4092, sizeof(short) * 2);
					if(NF[0]>2){
						readrid(ixfileHandle, rid, pageData, next, 2);
						readkey(ixfileHandle, attribute, rid, pageData, next, 2,key);
						page = next; // to the next page
						slot = 3;
						N = NF[0];
						return 0;
					}
					memcpy(&next, ((char*) pageData) + 12 + 8, sizeof(int));
				}
			}
		}
		free(pageData);
	}
	else{ //highKey != NULL
		void* pageData = malloc(PAGE_SIZE);
		ixfileHandle.fileHandle.readPage(page, pageData);
		int next;
		memcpy(&next, ((char*) pageData) + 12 + 8, sizeof(int));
		short NF[2];
		memcpy(NF, ((char*) pageData) + 4092, sizeof(short) * 2);

		if(slot != NF[0]){
			if (highKeyInclusive){ //true
				if(compare(attribute, slot, highKey, pageData) >= 0){
					readrid(ixfileHandle, rid, pageData, page, slot);
					readkey(ixfileHandle, attribute, rid, pageData, page, slot,key);
					//page is same
					slot++;
				}
				else{
					return -1;
				}
			}
			else{
				if(compare(attribute, slot, highKey, pageData) > 0){
					readrid(ixfileHandle, rid, pageData, page, slot);
					readkey(ixfileHandle, attribute, rid, pageData, page, slot,key);
					//page is same
					slot++;
				}
				else{
					return -1;
				}
			}
		}
		else{ // go to next page
			if(next == -1){
				return -1;
			}
			while (next != -1){
					//go to next page
					ixfileHandle.fileHandle.readPage(next, pageData);
					memcpy(NF, ((char*) pageData) + 4092, sizeof(short) * 2);
					if(NF[0]>2){
						if (highKeyInclusive){ //true
							if(compare(attribute, 2, highKey, pageData) >= 0){
								readrid(ixfileHandle, rid, pageData, next, 2);
								readkey(ixfileHandle, attribute, rid, pageData, next, 2,key);
								page = next; // to the next page
								slot = 3;
								N = NF[0];
								return 0;
							}
							else{
								return -1;
							}
						}
						else{ //false
							if(compare(attribute, 2, highKey, pageData) > 0){
								readrid(ixfileHandle, rid, pageData, next, 2);
								readkey(ixfileHandle, attribute, rid, pageData, next, 2,key);
								page = next; // to the next page
								slot = 3;
								N = NF[0];
								return 0;
							}
							else{
								return -1;
							}
						}
					}
					memcpy(&next, ((char*) pageData) + 12 + 8, sizeof(int));
				}
			}
		free(pageData);
		}
    return 0;
}

RC IX_ScanIterator::close()
{
    return PagedFileManager::instance()->closeFile(this->ixfileHandle.fileHandle);
}


IXFileHandle::IXFileHandle()
{
    ixReadPageCounter = 0;
    ixWritePageCounter = 0;
    ixAppendPageCounter = 0;
    used = 0;
    rootpage = 0;
}

IXFileHandle::~IXFileHandle()
{
}

RC IXFileHandle::collectCounterValues(unsigned &readPageCount, unsigned &writePageCount, unsigned &appendPageCount)
{
	this->ixReadPageCounter=this->fileHandle.readPageCounter;
	this->ixWritePageCounter=this->fileHandle.writePageCounter;
	this->ixAppendPageCounter=this->fileHandle.appendPageCounter;
	readPageCount=this->ixReadPageCounter;
	writePageCount=this->ixWritePageCounter;
	appendPageCount=this->ixAppendPageCounter;
    return 0;
}
