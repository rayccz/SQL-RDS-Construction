#include "rbfm.h"
#include <cstring>

RecordBasedFileManager* RecordBasedFileManager::_rbf_manager = 0;

RecordBasedFileManager* RecordBasedFileManager::instance()
{
    if(!_rbf_manager)
        _rbf_manager = new RecordBasedFileManager();

    return _rbf_manager;
}

RecordBasedFileManager::RecordBasedFileManager()
{
}

RecordBasedFileManager::~RecordBasedFileManager()
{
}

RC RecordBasedFileManager::createFile(const string &fileName) {
	//		//  record how many slot(records) in the page
	//		//
	//		//  [record1][record2][record3][record4]
	//		//
	//		//
	//		//	[N1][N2][N3][Numbers of slots(records)][Memory Left]
	//		// --------------My format------------
	//		//[Memory Left] : use 2 bytes to store how many memory left
	//		//[Numbers of slots] : use 2 bytes short
	//		//                N: each one is 4 bytes,
	//		//    first 2 bytes: pointer to specific position
	//		//     last 2 bytes: data length
	return PagedFileManager::instance()->createFile(fileName);
}

RC RecordBasedFileManager::destroyFile(const string &fileName) {
    return PagedFileManager::instance()->destroyFile(fileName);
}

RC RecordBasedFileManager::openFile(const string &fileName, FileHandle &fileHandle) {
	return PagedFileManager::instance()->openFile(fileName, fileHandle);
}

RC RecordBasedFileManager::closeFile(FileHandle &fileHandle) {
	return PagedFileManager::instance()->closeFile(fileHandle);
}

int RecordBasedFileManager::findInsertSlotPosition(void* pageData, int &tag){
	short N; // this page #slots
	memcpy(&N, ((char*)pageData) + 4092, sizeof(short));
	for(int i= 0; i < N; i++){
		short offset;
		// check each slot's offset in slotDirectory
		memcpy(&offset, ((char*)pageData) + 4092 - ((i+1) * 4), sizeof(short));
		if(offset == -1){
			tag = 1;
			return i;
		}
	}
	return N;
}

void RecordBasedFileManager::formatRecord(void* recordData, const void* data, const vector<Attribute> &recordDescriptor, short needSize){

	//make offset and add into the beginning of the record
	int fieldCount = recordDescriptor.size();
	short attributeOffset[fieldCount];		// each attribute has its offset (the length from record start to its end)

	int nullFieldsIndicatorActualSize = ceil((double) fieldCount / CHAR_BIT);
	int attr_pos = 0;
	int attr_pos_in_nth_byte = 0;
	int attr_pos_in_nth_bit_in_a_byte = 0;
	bool nullBit;
	int offset = 0;

	unsigned char *nullsIndicator = (unsigned char *) malloc(nullFieldsIndicatorActualSize);
	//get nullFieldIndicator to know whether the field exist!
	memcpy(nullsIndicator, data, nullFieldsIndicatorActualSize);
	offset += nullFieldsIndicatorActualSize;

	for(int i=0; i < fieldCount; i++){
		attr_pos++;
		attr_pos_in_nth_byte = floor((double) attr_pos / CHAR_BIT);
		attr_pos_in_nth_bit_in_a_byte = CHAR_BIT - attr_pos % CHAR_BIT;

		nullBit = nullsIndicator[attr_pos_in_nth_byte] & (1 << attr_pos_in_nth_bit_in_a_byte);
		if(!nullBit){
			int type = recordDescriptor.at(i).type;

			if(type == TypeInt){
				offset += sizeof(int);
				//					offset +  (record_header)              +  filedCount(4bytes)  +   indicator(char 1 byte)
				attributeOffset[i] = offset + (sizeof(short) * fieldCount) + sizeof(int)  + sizeof(char);

			}else if(type == TypeReal){
				offset += sizeof(float);
				attributeOffset[i] = offset + (sizeof(short) * fieldCount) + sizeof(int) + sizeof(char);
			}else if(type == TypeVarChar){
				void* nameLength = malloc(sizeof(int));
				memcpy(nameLength, (char*)data + offset, sizeof(int));
				offset += sizeof(int);

				int nameLengthSize = *((int*)nameLength);
				offset += sizeof(char) * nameLengthSize;
				attributeOffset[i] = offset + (sizeof(short) * fieldCount) + sizeof(int) + sizeof(char);

				free(nameLength);
			}
		}else{
			//means this field is null
			//attributeOffset[i] = -1;
			attributeOffset[i] = offset;
		}
	}

	//write to recordData (indicator, fieldCount, offset"s", nullIndicator, data)
	char indicator = 0; // this record is not a RID
	offset = 0;

	memcpy((char*)recordData, &indicator, sizeof(char));
	offset += sizeof(char);

	memcpy((char*)recordData + offset, &fieldCount, sizeof(int));
	offset += sizeof(int);

	memcpy((char*)recordData + offset, attributeOffset, sizeof(short) * fieldCount);
	offset += sizeof(short) * fieldCount;

	//															slotDirectory(4bytes)
	memcpy((char*)recordData + offset, data, needSize - offset - 4);

	free(nullsIndicator);
}

RC RecordBasedFileManager::IXinsertRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const void *data, RID &rid, int page) {
	short needSpace = getNeededInsertRecordSize(recordDescriptor, data);

	int s;
	memcpy(&s,(char*)data + 1,4);

	//read pageData
	void* pageData = malloc(PAGE_SIZE);
	fileHandle.readPage(page, pageData);

	short NF2[2];
	memcpy(NF2, ((char*) pageData) + 4092, sizeof(short) * 2);

	//find the insert position
	short slotsNums = getPageSlotNumber(page, fileHandle);
	short slotStartPosition = getSlotStartPosition(pageData, slotsNums);

	void* recordData = malloc(needSpace - 4); // minus 4
	formatRecord(recordData, data, recordDescriptor, needSpace);

	//														needSpace - slotDirectory(4bytes)
	memcpy(((char*)pageData) + slotStartPosition, recordData, needSpace - 4);

	//append slot directory in end of the page


	memcpy(NF2, ((char*) pageData) + 4092, sizeof(short) * 2);

	int tag = 0; // tag to check whether reuse the deleteSlotDirectory
	int recordSlotPosition = findInsertSlotPosition(pageData, tag);
	short slotInfo[2] = {slotStartPosition, (short)(needSpace - 4)};
	memcpy(((char*)pageData) + PAGE_SIZE - ((recordSlotPosition + 1) * 4) - (sizeof(short) * 2), slotInfo, sizeof(short) * 2);

	//update N and F, only get F because we don't have F but have N
	short freeSizeOfThisPage;
	memcpy(&freeSizeOfThisPage, ((char*)pageData) + 4094, sizeof(short));

	if(tag != 1)
		slotsNums++;

	short NF[2] = {(short)(slotsNums), (short)(freeSizeOfThisPage - needSpace)};
	//short NF[2] = {(short)(slotsNums + 1), (short)(freeSizeOfThisPage - needSpace)};
	memcpy(((char*)pageData) + 4092, NF, sizeof(short) * 2);

	//finally write back
	fileHandle.writePage(page, pageData);

	rid.pageNum = page;
	rid.slotNum = recordSlotPosition;

	free(recordData);
	free(pageData);
	return 0;
}

RC RecordBasedFileManager::insertRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const void *data, RID &rid) {
	short needSpace = getNeededInsertRecordSize(recordDescriptor, data);
	int page = getPageHasSapce(needSpace, fileHandle);

	//no enough space, need to append page
	if(page == -1){
		fileHandle.appendPage2();
	}

	//update page, because may be changed from -1 to n due to appendPage2
	page = getPageHasSapce(needSpace, fileHandle);

	//read pageData
	void* pageData = malloc(PAGE_SIZE);
	fileHandle.readPage(page, pageData);

	//find the insert position
	short slotsNums = getPageSlotNumber(page, fileHandle);
	short slotStartPosition = getSlotStartPosition(pageData, slotsNums);

	void* recordData = malloc(needSpace - 4); // minus 4
	formatRecord(recordData, data, recordDescriptor, needSpace);

	//														needSpace - slotDirectory(4bytes)
	memcpy(((char*)pageData) + slotStartPosition, recordData, needSpace - 4);

	//append slot directory in end of the page

	int tag = 0; // tag to check whether reuse the deleteSlotDirectory
	int recordSlotPosition = findInsertSlotPosition(pageData, tag);
	short slotInfo[2] = {slotStartPosition, (short)(needSpace - 4)};
	memcpy(((char*)pageData) + PAGE_SIZE - ((recordSlotPosition + 1) * 4) - (sizeof(short) * 2), slotInfo, sizeof(short) * 2);

	//update N and F, only get F because we don't have F but have N
	short freeSizeOfThisPage;
	memcpy(&freeSizeOfThisPage, ((char*)pageData) + 4094, sizeof(short));

	if(tag != 1)
		slotsNums++;

	short NF[2] = {(short)(slotsNums), (short)(freeSizeOfThisPage - needSpace)};
	//short NF[2] = {(short)(slotsNums + 1), (short)(freeSizeOfThisPage - needSpace)};
	memcpy(((char*)pageData) + 4092, NF, sizeof(short) * 2);

	//finally write back
	fileHandle.writePage(page, pageData);

	rid.pageNum = page;
	rid.slotNum = recordSlotPosition;

	free(recordData);
	free(pageData);
	return 0;
}

short RecordBasedFileManager::getSlotStartPosition(void* pageData, int pageSlots){
	if(pageSlots == 0){
		return 0;
	}else{
		int end = 0;
		for(int i = 1; i <= pageSlots; i++){
			short slotInfo[2];
			memcpy(slotInfo, ((char*)pageData) + PAGE_SIZE - (i * 4) - (sizeof(short) * 2), sizeof(short) * 2);

			if(slotInfo[0] == -1)
				continue;
			end = max(end, slotInfo[0] + slotInfo[1]);
		}
		return (short)end;
	}
}

short RecordBasedFileManager::getPageSlotNumber(int page, FileHandle &fileHandle){
	if(page == -1)
		return -1;
	else{
		//			            number of page    + move to check N
		fseek(fileHandle.file, (page * PAGE_SIZE) + 4092, SEEK_SET);
		short slotsNums;
		fread(&slotsNums, sizeof(short), 1, fileHandle.file);
		return slotsNums;
	}
}

int RecordBasedFileManager::getPageHasSapce(int needSpace, FileHandle &fileHandle){
	for(int i = 0; i < fileHandle.getNumberOfPages(); i++){
		void* pageData = malloc(PAGE_SIZE);
		fileHandle.readPage(i, pageData);
		short leftMemory;
		memcpy(&leftMemory, ((char*)pageData) + 4094, sizeof(short));

		//free memory
		free(pageData);
		if(leftMemory >= needSpace){
			return i;
		}
	}
	// existing pages are all full
	return -1;
}

short RecordBasedFileManager::getNeededInsertRecordSize(const vector<Attribute> &recordDescriptor, const void *data){
	int fieldCount = recordDescriptor.size();
	int nullFieldsIndicatorActualSize = ceil((double) fieldCount / CHAR_BIT);

	bool nullBit;
	int offset = 0;
	int neededSize = 0;

	unsigned char *nullsIndicator = (unsigned char *) malloc(nullFieldsIndicatorActualSize);
	//get nullFieldIndicator to know whether the field exist!
	memcpy(nullsIndicator, data, nullFieldsIndicatorActualSize);
	offset += nullFieldsIndicatorActualSize;
	neededSize += nullFieldsIndicatorActualSize;

    int attr_pos = 0;
    int attr_pos_in_nth_byte = 0;
    int attr_pos_in_nth_bit_in_a_byte = 0;

    for(int i=0; i < fieldCount; i++){
		attr_pos++;
		attr_pos_in_nth_byte = floor((double) attr_pos / CHAR_BIT);
		attr_pos_in_nth_bit_in_a_byte = CHAR_BIT - attr_pos % CHAR_BIT;

		nullBit = nullsIndicator[attr_pos_in_nth_byte] & (1 << attr_pos_in_nth_bit_in_a_byte);

		// 8 bits = 1 bytes
		//nullBit = nullsIndicator[0] & (1 << ( ((8 * nullFieldsIndicatorActualSize) - 1) - i) );

		if(!nullBit){
			int type = recordDescriptor.at(i).type;

			if(type == TypeInt){
				offset += sizeof(int);
				neededSize += sizeof(int);
			}else if(type == TypeReal){
				offset += sizeof(float);
				neededSize += sizeof(float);
			}else if(type == TypeVarChar){
				void* nameLength = malloc(sizeof(int));
				memcpy(nameLength, (char*)data + offset, sizeof(int));
				offset += sizeof(int);
				neededSize += sizeof(int);
				int nameLengthSize = *((int*)nameLength);

				offset += sizeof(char) * nameLengthSize;
				neededSize += sizeof(char) * nameLengthSize;
				free(nameLength);
			}
		}
	}

	//free memory
	free(nullsIndicator);

	//     total size + slotDirectory       +  fieldCount +      attributeOffset         +   indicator(check whether is RID or not)
	return neededSize + (sizeof(short) * 2) + sizeof(int) + (sizeof(short) * fieldCount) + sizeof(char);
}


RC RecordBasedFileManager::readRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid, void *data) {
	//check pageNum
	int page = rid.pageNum;
	if(page > fileHandle.getNumberOfPages() - 1){
		return -1;
    }

	//check slotNum
	int slot = rid.slotNum;

	void* pageData = malloc(PAGE_SIZE);
	fileHandle.readPage(page, pageData);

	short slotInfo[2];
	memmove(slotInfo, ((char*)pageData) + 4096 - ((slot + 1) * 4) - (sizeof(short) * 2) , sizeof(short) * 2);

	if(slotInfo[0] == -1){
		return -1;
	}

	//check indicator
	char indicator;
	memmove(&indicator, ((char*)pageData) + slotInfo[0], sizeof(char));
	if(indicator == 1){
		int pageNum;
		short slotNum;
		memmove(&pageNum, ((char*)pageData) + slotInfo[0] + sizeof(char), sizeof(int));
		memmove(&slotNum, ((char*)pageData) + slotInfo[0] + sizeof(char) + sizeof(int), sizeof(short));
		RID newRID;
		newRID.pageNum = pageNum;
		newRID.slotNum = slotNum;
		return readRecord(fileHandle, recordDescriptor, newRID, data);
	}else{
		int fieldCount = recordDescriptor.size();
		//					dataPage   + slotStartPos+ indicator  + fieldCount +  attributeOffset
		memmove(data, ((char*)pageData) + slotInfo[0] + sizeof(char) + sizeof(int) + (sizeof(short) * fieldCount), slotInfo[1] - sizeof(int) - (sizeof(short) * fieldCount));
	}

	free(pageData);
	return 0;
}

RC RecordBasedFileManager::printRecord(const vector<Attribute> &recordDescriptor, const void *data) {
	if(recordDescriptor.empty() || (recordDescriptor.size() == 0) || data == NULL )
		return -1;
	else{
		int fieldCount = recordDescriptor.size();
		int nullFieldsIndicatorActualSize = ceil((double) fieldCount / CHAR_BIT);

		bool nullBit;
		int offset = 0;

		unsigned char *nullsIndicator = (unsigned char *) malloc(nullFieldsIndicatorActualSize);
		//get nullFieldIndicator to know whether the field exist!
		memcpy(nullsIndicator, data, nullFieldsIndicatorActualSize);
		offset += nullFieldsIndicatorActualSize;

		int attr_pos = 0;
		int attr_pos_in_nth_byte = 0;
		int attr_pos_in_nth_bit_in_a_byte = 0;
		for(int i=0; i < fieldCount; i++){
			attr_pos++;
			attr_pos_in_nth_byte = floor((double) attr_pos / CHAR_BIT);
			attr_pos_in_nth_bit_in_a_byte = CHAR_BIT - attr_pos % CHAR_BIT;

			// 8 bits = 1 bytes
			nullBit = nullsIndicator[attr_pos_in_nth_byte] & (1 << attr_pos_in_nth_bit_in_a_byte);

			if(!nullBit){
				int type = recordDescriptor.at(i).type;

				if(type == TypeInt){
					void* value1 = malloc(sizeof(int));
					memcpy(value1, (char*)data + offset, sizeof(int));
					cout << i << "." << recordDescriptor.at(i).name << ":" << *((int*)value1) << endl;
					offset += sizeof(int);
					//free memory
					free(value1);
				}else if(type == TypeReal){
					void* value2 = malloc(sizeof(float));
					memcpy(value2, (char*)data + offset, sizeof(float));
					cout << i << "." << recordDescriptor.at(i).name << ":" << *((float*)value2) << endl;
					offset += sizeof(float);
					//free memory
					free(value2);
				}else if(type == TypeVarChar){
					void* nameLength = malloc(sizeof(int));
					memcpy(nameLength, (char*)data + offset, sizeof(int));
					offset += sizeof(int);

					int nameLengthSize = *((char*)nameLength);

					void* value3 = malloc(sizeof(char) * nameLengthSize);
					memcpy(value3, (char*)data + offset, sizeof(char) * nameLengthSize);
					cout << i << "." << recordDescriptor.at(i).name << ":" << (char*)value3 << endl;
					offset += sizeof(char) * nameLengthSize;
					//free memory
					free(value3);
					free(nameLength);
				}
			}
		}
		//free memory
		free(nullsIndicator);
		return 0;
	}
}

RC RecordBasedFileManager::updateRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const void *data, const RID &rid){
	/*
	 * 1. check not over index
	 * 2. determine how to deal with data
	 * 3. doing...
	 */

	// ===== part1 start =====
	//check pageNum
	int pageNum = fileHandle.getNumberOfPages();
	if(rid.pageNum > pageNum)
		return - 1;

	//get pageData
	void* pageData = malloc(PAGE_SIZE);
	fileHandle.readPage(rid.pageNum, pageData);

	//get NF
	short NF[2]; //NF[0] = N,  NF[1] = F
	memmove(NF, ((char*)pageData) + 4092, sizeof(short) * 2);

	if(rid.slotNum > NF[0] - 1){
		return -1;
	}

	// ===== part2, 3 start =====
	short needSize = getNeededInsertRecordSize(recordDescriptor, data);

	//get slotInfo
	short slotInfo[2]; // slotInfo[0] : offset     slotInfo[1] : length
	memmove(slotInfo, ((char*)pageData) + 4092 - (rid.slotNum + 1) * 4, sizeof(short) * 2);

	//deleted record
	if(slotInfo[0] == -1){
		return -1;
	}

	char indicator;
	memmove(&indicator, ((char*)pageData) + slotInfo[0], sizeof(char));
	if(indicator == 1){
		RID tempRID;
		int tempPageNum;
		short tempSlotNum;
		memmove(&tempPageNum, ((char*)pageData) + slotInfo[0] + 1, sizeof(int));
		memmove(&tempSlotNum, ((char*)pageData) + slotInfo[0] + 1 + 4, sizeof(short));
		tempRID.pageNum = tempPageNum;
		tempRID.slotNum = tempSlotNum;
		return updateRecord(fileHandle, recordDescriptor, data, tempRID);
	}

	short newRecordSize = needSize - 4;
	short oldRecordSize = slotInfo[1];
	int freeSpace = NF[1];
	int slotNums = NF[0];

	// directly replace original data
	if(newRecordSize == oldRecordSize){
		void* newFormattedRecord = malloc(newRecordSize);
		formatRecord(newFormattedRecord, data, recordDescriptor, needSize);
		memmove(((char*)pageData) + slotInfo[0], newFormattedRecord, newRecordSize);
		free(newFormattedRecord);
	}else if(newRecordSize < oldRecordSize){
		/*
		 * 1. shift left
		 * 2. replace oldRecord
		 * 3. update NF
		 * 4. update slotDirectory
		 * */

		//shift
		int shiftNum = PAGE_SIZE - 4 - (slotNums * 4) - slotInfo[0] - oldRecordSize; //shiftNum =  pageSize - NF - slotInfo[0] - slotDirectory
		memmove( ((char*)pageData) + slotInfo[0] + newRecordSize, ((char*)pageData) + slotInfo[0] + oldRecordSize, shiftNum);

		//replace
		void* recordData = malloc(newRecordSize);
		formatRecord(recordData, data, recordDescriptor, needSize);
		memmove(((char*)pageData) + slotInfo[0], recordData, newRecordSize);

		//update NF (add free size)
		NF[1] = NF[1] + (oldRecordSize - newRecordSize);
		memmove(((char*)pageData) + 4092, NF, sizeof(short) * 2);

		//update slotDirectory
		for(int i = 1; i <= NF[0]; i++){
			short tempSlotInfo[2];
			memmove(tempSlotInfo, ((char*)pageData) + 4092 - (i * 4), sizeof(short) * 2);
			// 2 condition need to skip,  1: slot is deleted   2:
			if(tempSlotInfo[0] == -1 || tempSlotInfo[0] < slotInfo[0] || i == (rid.slotNum + 1)){
				continue;
			}
			tempSlotInfo[0] = tempSlotInfo[0] - (oldRecordSize - newRecordSize);
			memmove(((char*)pageData) + 4092 - (i * 4), tempSlotInfo, sizeof(short) * 2);
		}
		//update "this" record directory
		slotInfo[1] = newRecordSize;
		memmove(((char*)pageData) + 4092 - (rid.slotNum + 1) * 4, slotInfo, sizeof(short) * 2);
		free(recordData);
	}else{
		if((newRecordSize - oldRecordSize) > freeSpace){
			//make this record become a RID
			/*
			 * 1. shift left
		  	 * 2. replace oldRecord
			 * 3. update NF
			 * 4. update slotDirectory
			* */

			//shift left
			int shiftNum = PAGE_SIZE - 4 - (slotNums * 4) - slotInfo[0] - oldRecordSize; //shiftNum =  pageSize - NF - slotInfo[0] - slotDirectory
			memmove(((char*)pageData) + slotInfo[0] + 7, ((char*)pageData) + slotInfo[0] + slotInfo[1], shiftNum);

			// create replaceRecord to replace original record
			RID replaceRID;
			this->insertRecord(fileHandle, recordDescriptor, data, replaceRID);

			void* replaceRecord = malloc(7); // 1byte indicator, 4bytes pageNum, 2bytes slotNum
			char indicator = 1;
			memmove(replaceRecord, &indicator, sizeof(char));

			int replacePage = replaceRID.pageNum;
			short replaceSlotNum = replaceRID.slotNum;
			memmove(((char*)replaceRecord) + 1, &replacePage, sizeof(int));
			memmove(((char*)replaceRecord) + 1 + 4, &replaceSlotNum, sizeof(short));
			//replace
			memmove(((char*)pageData) + slotInfo[0], replaceRecord, 7);

			//update NF (add free size)
			NF[1] = NF[1] + (oldRecordSize - 7);
			memmove(((char*)pageData) + 4092, NF, sizeof(short) * 2);

			//update slotDirectory
			for(int i = 1; i <= NF[0]; i++){
				short tempSlotInfo[2];
				memmove(tempSlotInfo, ((char*)pageData) + 4092 - (i * 4), sizeof(short) * 2);
				// 2 condition need to skip,  1: slot is deleted   2:
				if(tempSlotInfo[0] == -1 || tempSlotInfo[0] < slotInfo[0] || i == (rid.slotNum + 1)){
					continue;
				}

				tempSlotInfo[0] = tempSlotInfo[0] - (oldRecordSize - 7);
				memmove(((char*)pageData) + 4092 - (i * 4), tempSlotInfo, sizeof(short) * 2);
			}
			//update "this" record directory
			slotInfo[1] = 7;
			memmove(((char*)pageData) + 4092 - (rid.slotNum + 1) * 4, slotInfo, sizeof(short) * 2);
			free(replaceRecord);
		}else{
			// enough space, shift right
			/*
			 * 1. shift right
			 * 2. replace oldRecord
			 * 3. update NF
			 * 4. update slotDirectory
			* */

			//shift right
			int shiftNum = PAGE_SIZE - 4 - (slotNums * 4) - slotInfo[0] - newRecordSize; //shiftNum =  pageSize - NF - slotInfo[0] - slotDirectory
			memmove(((char*)pageData) + slotInfo[0] + newRecordSize, ((char*)pageData) + slotInfo[0] + oldRecordSize, shiftNum);

			//replace
			void* recordData = malloc(newRecordSize);
			formatRecord(recordData, data, recordDescriptor, needSize);
			memmove(((char*)pageData) + slotInfo[0], recordData, newRecordSize);

			//update NF
			NF[1] = NF[1] - (newRecordSize - oldRecordSize);
			memmove(((char*)pageData) + 4092, NF, sizeof(short) * 2);

			//update slotDirectory
			for(int i = 1; i <= NF[0]; i++){
				short tempSlotInfo[2];
				memmove(tempSlotInfo, ((char*)pageData) + 4092 - (i * 4), sizeof(short) * 2);
				// 2 condition need to skip,  1: slot is deleted   2:slot before
				if(tempSlotInfo[0] == -1 || tempSlotInfo[0] < slotInfo[0] || i == (rid.slotNum + 1)){
					continue;
				}

				tempSlotInfo[0] = tempSlotInfo[0] + (newRecordSize - oldRecordSize);
				memmove(((char*)pageData) + 4092 - (i * 4), tempSlotInfo, sizeof(short) * 2);
			}
			//update "this" record directory
			slotInfo[1] = newRecordSize;
			memmove(((char*)pageData) + 4092 - (rid.slotNum + 1) * 4, slotInfo, sizeof(short) * 2);

			free(recordData);
		}
	}

	//update whole page
	fileHandle.writePage(rid.pageNum, pageData);

	free(pageData);
	return 0;
}


RC RecordBasedFileManager::deleteRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid){
	/*
	 * 1. check not over index
	 * 2. move record forward (need to check whether it is a RID)
	 * 3. update slotDirectory (shifting)
	 * 4. update N and F
	 * */

	// =====part1  start =====
	//check pageNum
	int pageNum = fileHandle.getNumberOfPages();
	if(rid.pageNum > pageNum){
		return - 1;
	}
	//get pageData
	void* pageData = malloc(PAGE_SIZE);
	fileHandle.readPage(rid.pageNum, pageData);
	short NF[2];
	memmove(NF, ((char*)pageData) + 4092, sizeof(short) * 2);

	if(rid.slotNum > NF[0]){
		return -1;
	}

	// =====part2  start =====
	//get slotInfo
	short slotInfo[2]; // slotInfo[0] : offset     slotInfo[1] : length
	memmove(slotInfo, ((char*)pageData) + 4092 - (rid.slotNum + 1) * 4, sizeof(short) * 2);

	//already delete
	if(slotInfo[0] == -1){
		return 0;
	}

	// check whether this record is a RID
	void* recordData = malloc(slotInfo[1]);
	memmove(recordData, ((char*)pageData) + slotInfo[0], slotInfo[1]);

	char indicator;
	memmove(&indicator, recordData, sizeof(char));

	if(indicator == 1){
		RID deleteRID;
		int deletePageNum;
		short deleteSlotNum;

		memmove(&deletePageNum, ((char*)pageData) + slotInfo[0] + 1, sizeof(int));
		memmove(&deleteSlotNum, ((char*)pageData) + slotInfo[0] + 5, sizeof(short));
		deleteRID.pageNum = deletePageNum;
		deleteRID.slotNum = deleteSlotNum;
		deleteRecord(fileHandle, recordDescriptor, deleteRID);
	}

	//move data forward
	short moveNum = PAGE_SIZE - (NF[0] * 4) - 4 - (slotInfo[0] + slotInfo[1]);
	memmove(((char*)pageData + slotInfo[0]), ((char*)pageData) + (slotInfo[0] + slotInfo[1]), moveNum);

	// =====part3  start =====

	//update slotDirectory
	for(int i = 1; i <= NF[0]; i++){
		short tempSlotInfo[2];
		memmove(tempSlotInfo, ((char*)pageData) + 4092 - (i * 4), sizeof(short) * 2);
		// 2 condition need to skip,  1: slot is deleted   2:slot before
		if(tempSlotInfo[0] == -1 || tempSlotInfo[0] < slotInfo[0] || i == (rid.slotNum + 1)){
			continue;
		}

		tempSlotInfo[0] = tempSlotInfo[0] - slotInfo[1];
		memmove(((char*)pageData) + 4092 - (i * 4), tempSlotInfo, sizeof(short) * 2);
	}

	//finally modify delete Data
	slotInfo[0] = -1;
	memmove(((char*)pageData) + 4092 - ((rid.slotNum + 1) * 4) , slotInfo, sizeof(short) * 2);

	// =====part4  start =====
//	NF[0] = NF[0] - 1;  no need to minus after it added
	NF[1] = NF[1] + slotInfo[1];  //  (recordSize) and (slotDirectorySize)
	memmove(((char*)pageData) + 4092, NF, sizeof(short) * 2);
	// =====part4  end =====

	fileHandle.writePage(rid.pageNum, pageData);

	free(pageData);
	free(recordData);
	return 0;
}

RC RecordBasedFileManager::scan(FileHandle &fileHandle,
      const vector<Attribute> &recordDescriptor,
      const string &conditionAttribute,
      const CompOp compOp,                  // comparision type such as "<" and "="
      const void *value,                    // used in the comparison
      const vector<string> &attributeNames, // a list of projected attributes
      RBFM_ScanIterator &rbfm_ScanIterator){

	rbfm_ScanIterator.fileHandle=fileHandle;
	rbfm_ScanIterator.recordDescriptor=recordDescriptor;
	rbfm_ScanIterator.conditionAttribute=conditionAttribute;
	rbfm_ScanIterator.compOp=compOp;
	rbfm_ScanIterator.value=((char*) value);
	rbfm_ScanIterator.attributeNames=attributeNames;

	return 0;
}

RC RBFM_ScanIterator::getNextRID(FileHandle &fileHandle,
		const vector<Attribute> &recordDescriptor,
		const string &conditionAttribute,
		const CompOp compOp, 				 // comparison type such as "<" and "="
		const void *value,                    // used in the comparison
		const vector<string> &attributeNames, // a list of projected attributes
		RBFM_ScanIterator &rbfm_ScanIterator) {
	//get which attribute will be used for compare
	int compare = 0;
	int comparetype;
	for (int i = 0; i < recordDescriptor.size(); i++) {
		if (recordDescriptor[i].name == conditionAttribute) {
			compare = i;
			if (recordDescriptor[i].type == 0) {
				comparetype = 0;
			} else if (recordDescriptor[i].type == 1) {
				comparetype = 1;
			} else {
				comparetype = 2; //varchar
			}
		}
	}
//	cout << "compare:" << compare << endl;
//	cout << "conditionAttribute:" << conditionAttribute << endl;
//	cout << "compOp:" << compOp << endl;

//	cout << "before get number" << endl;
	int page = fileHandle.getNumberOfPages();
//	cout << "after get number" << endl;
	void* pageData = malloc(PAGE_SIZE);

	int check = 0;
	// for all pages
	int j = rbfm_ScanIterator.rid.slotNum;
	for (int i = rbfm_ScanIterator.rid.pageNum; i < page; i++) {
		fileHandle.readPage(i, pageData);

		short slotsNums;
		memcpy(&slotsNums, ((char*) pageData) + 4096 - 4, sizeof(short));

		if (check == 1) {
			break;
		}

		if(i>rbfm_ScanIterator.rid.pageNum){
			rbfm_ScanIterator.rid.slotNum=0;
		}
		// for all slots in the page
		for (j = rbfm_ScanIterator.rid.slotNum; j < slotsNums; j++) {
			//record the current page number and slot number

			// get the position of the slot
			short addr;
			memcpy(&addr, ((char*) pageData) + 4092 - ((j+1) * 4), sizeof(short));

			// for each record
			char indicator;  // means this record not a RID
			memcpy(&indicator, ((char*) pageData) + addr, sizeof(char)); //indicator

			if (addr != -1 && indicator == 0) {
				int fildcount;
				memcpy(&fildcount, ((char*) pageData) + addr + 1, sizeof(int));

				int less=compare % 8;
				int div=compare / 8;

				//always 1 byte?
				unsigned char *nullcheck = (unsigned char *) malloc(sizeof(unsigned char));
				memcpy(nullcheck, ((char*)pageData) +addr + 1 + sizeof(int) + (fildcount * sizeof(short)) +div, sizeof(unsigned char));
				bool isNull;
				isNull = nullcheck[0] & (1 << (7 - less));

				int pos = 0;

				if (compare == 0) { //the first attribute
					//need to know the nullindicator size
					int nullbyte = 0;
					if (recordDescriptor.size() % 8 == 0) {
						nullbyte = recordDescriptor.size() / 8;
					} else {
						nullbyte = recordDescriptor.size() / 8 + 1;
					}
					pos = 1 + sizeof(int) + fildcount * sizeof(short) + nullbyte; //indicator + fieldCount(4bytes) + all slots + nullindicator
				}else{
					short offset;
					memcpy(&offset, ((char*)pageData) + addr + 1 + sizeof(int) + (compare -1) * sizeof(short), sizeof(short));
					pos += offset;
				}


				int intcompare;
				float realcompare;
				string stringcompare = "";

				int intattr;
				float realattr;
				string stringattr;

				if (compOp != NO_OP && comparetype == 0 ) { //int
					memcpy(&intcompare, ((char*) pageData) + addr + pos, sizeof(int));
					memcpy(&intattr, ((char*) value), sizeof(int));

				} else if (compOp != NO_OP && comparetype == 1) { //real
					memcpy(&realcompare, ((char*) pageData) + addr + pos, sizeof(int));
					memcpy(&realattr, ((char*) value), sizeof(float));
				} else if (compOp != NO_OP && comparetype == 2){ //char
					int length;
					memcpy(&length, ((char*) pageData) + addr + pos, sizeof(int));

					char c;
					for (int k = 0; k < length; k++) {
						memcpy(&c, ((char*) pageData) + addr + pos + 4 + k, sizeof(char));
						stringcompare = stringcompare + c;
					}

					memcpy(&length, ((char*) value), sizeof(int));

					for (int k = 0; k < length; k++) {
						memcpy(&c, ((char*) value) + 4 + k, sizeof(char));
						stringattr = stringattr + c;
					}
				}


				if (compOp == NO_OP) { //no condition
					check = 1;
				} else if (compOp == EQ_OP && !isNull ) { // =
					if (comparetype == 0) {
						if (intattr == intcompare) {
							check = 1;
						}
					} else if (comparetype == 1 ) {
						if (realattr == realcompare) {
							check = 1;
						}
					} else {
						if (stringattr == stringcompare) {
							check = 1;
						}
					}
				} else if (compOp == LT_OP && !isNull ) { // >
					if (comparetype == 0) {
						if (intattr > intcompare) {
							check = 1;
						}
					} else if (comparetype == 1) {
						if (realattr > realcompare) {
							check = 1;
						}
					} else {
						if (stringattr > stringcompare) {
							check = 1;
						}
					}
				} else if (compOp == LE_OP && !isNull) { // >=
					if (comparetype == 0) {
						if (intattr >= intcompare) {
							check = 1;
						}
					} else if (comparetype == 1) {
						if (realattr >= realcompare) {
							check = 1;
						}
					} else {
						if (stringattr >= stringcompare) {
							check = 1;
						}
					}
				} else if (compOp == GT_OP && !isNull ) { // <
					if (comparetype == 0) {
						if (intattr < intcompare) {
							check = 1;
						}
					} else if (comparetype == 1) {
						if (realattr < realcompare) {
							check = 1;
						}
					} else {
						if (stringattr < stringcompare) {
							check = 1;
						}
					}
				} else if (compOp == GE_OP && !isNull) { // <=
					if (comparetype == 0) {
						if (intattr <= intcompare) {
							check = 1;
						}
					} else if (comparetype == 1) {
						if (realattr <= realcompare) {
							check = 1;
						}
					} else {
						if (stringattr <= stringcompare) {
							check = 1;
						}
					}
				} else if (compOp == NE_OP && !isNull) { // !=
					if (comparetype == 0) {
						if (intattr != intcompare) {
							check = 1;
						}
					} else if (comparetype == 1) {
						if (realattr != realcompare) {
							check = 1;
						}
					} else {
						if (stringattr != stringcompare) {
							check = 1;
						}
					}
				}

			}
			if (check == 1) {
				rbfm_ScanIterator.rid.pageNum = i;
				rbfm_ScanIterator.rid.slotNum = j;
				break;
			}
		}
	}
	if (check == 0) { //no match found
		rbfm_ScanIterator.rid.slotNum = 4096;
	}

	free(pageData);
	return 0;
}

