#include "pfm.h"

//below
#include <iostream>
#include <stdio.h>
using namespace std;
#include <sys/stat.h>
//above


PagedFileManager* PagedFileManager::_pf_manager = 0;

PagedFileManager* PagedFileManager::instance()
{
    if(!_pf_manager)
        _pf_manager = new PagedFileManager();

    return _pf_manager;
}


PagedFileManager::PagedFileManager()
{
}


PagedFileManager::~PagedFileManager()
{
}


RC PagedFileManager::createFile(const string &fileName)
{
	FILE * file;
	file = fopen (fileName.c_str(), "r");
	//already exist
	if (file){
		//cout << "exist!" << endl;
		fclose (file);
		return -1;
	}else{
		file = fopen (fileName.c_str(), "w+");
		fclose (file);
		return 0;
	}

}


RC PagedFileManager::destroyFile(const string &fileName)
{
	if(remove(fileName.c_str()) == 0)
		return 0;
	else
		return -1;
}


RC PagedFileManager::openFile(const string &fileName, FileHandle &fileHandle)
{
	FILE * file;
	file = fopen (fileName.c_str(), "r+");
	if(file != NULL){
		fileHandle.file = file;
		fileHandle.fileName = fileName;
		return 0;
	}

    return -1;
}


RC PagedFileManager::closeFile(FileHandle &fileHandle)
{
	if(fileHandle.file == NULL)
		return -1;
	else
		fclose(fileHandle.file);
	return 0;
//	else{
//		//update counter number
//		FILE* file = fopen(fileHandle.fileName.c_str(), "r+");
//		unsigned int counter[3] = {fileHandle.readPageCounter, fileHandle.writePageCounter, fileHandle.appendPageCounter};
//		fwrite(counter, sizeof(int) * HEADER_NUM, 1, file);
//		fclose(fileHandle.file);
//		fileHandle.file = NULL;
//		fileHandle.fileName.clear();
//	}
}


FileHandle::FileHandle()
{
    readPageCounter = 0;
    writePageCounter = 0;
    appendPageCounter = 0;
    file = NULL;
}


FileHandle::~FileHandle()
{
}


RC FileHandle::readPage(PageNum pageNum, void *data)
{
	if(file == NULL || data == NULL)
		return -1;
	else{
		// index starts from zero, and need to count hidden page
		if(pageNum <= getNumberOfPages() - 1){
			//file = fopen (fileName.c_str(), "r");

			//move position, then read (page size + header size)
			//fseek(file, pageNum * PAGE_SIZE + HEADER_OFFSET, SEEK_SET);
			fseek(file, pageNum * PAGE_SIZE, SEEK_SET);
			fread(data, PAGE_SIZE, 1, file);
			this->readPageCounter += 1;
			//fclose (file);
			return 0;
		}else{
			return -1;
		}
	}
}


RC FileHandle::writePage(PageNum pageNum, const void *data)
{
    if(data != NULL && this->checkFileExist() == 1 && getNumberOfPages() - 1 >= pageNum){
		//move position, then write  (page size + header size)
		//fseek(file, pageNum * PAGE_SIZE + HEADER_OFFSET, SEEK_SET);
    		fseek(file, pageNum * PAGE_SIZE, SEEK_SET);
		fwrite(data, PAGE_SIZE, 1, file);
		this->writePageCounter += 1;
		//fflush(file);
		//fclose (file);
		return 0;
    }else{
    		return -1;
    }
}


RC FileHandle::appendPage(const void *data)
{
	if(data != NULL && checkFileExist() == 1){
		//file = fopen(fileName.c_str(), "r+");
		fseek(file, 0, SEEK_END);
		fwrite((char*)data, PAGE_SIZE, 1, file);
		fflush(file);
		this-> appendPageCounter += 1;
		//fclose (file);
		return 0;
	}else{
		return -1;
	}
}

void FileHandle::appendPage2(){
	void* uselessData = malloc(PAGE_SIZE);
	for(unsigned j = 0; j < 100; j++){
		for(unsigned i = 0; i < PAGE_SIZE; i++){
			*((char *)uselessData+i) = i % (j+1) + 30;
		}
	}

	//move to pages_size, then append
	fseek(file, 0, SEEK_END);
	fwrite((char*)uselessData, PAGE_SIZE, 1, file);
	initPage();
	this-> appendPageCounter += 1;

	free(uselessData);
}

void FileHandle::initPage(){
	/*
	 * page format
	 *
	 * r1  r2  r3
	 *
	 *
	 * __(4) __(4) __(4)  N(2)  F(2)
	 *
	 * */
	short leftMemory = 4092;   //  PAGE_SIZE - 2(n) - 2(F)
	short numbersOfRecords = 0;
	//set N
	//fseek(file, HEADER_OFFSET + (this->appendPageCounter) * PAGE_SIZE + 4092, SEEK_SET);
	fseek(file, (this->getNumberOfPages() - 1) * PAGE_SIZE + 4092, SEEK_SET);
	fwrite(&numbersOfRecords, sizeof(short), 1, file);
	//set F
	//fseek(file, HEADER_OFFSET + (this->appendPageCounter) * PAGE_SIZE + 4094, SEEK_SET);
	fseek(file, (this->getNumberOfPages() - 1) * PAGE_SIZE + 4094, SEEK_SET);
	fwrite(&leftMemory, sizeof(short), 1, file);
	fflush(file);
}

RC FileHandle::checkFileExist(){
	if(fileName.empty())
		return -1;
	FILE* fileTemp = fopen(fileName.c_str(), "r");
	if(fileTemp){
		fclose(fileTemp);
		return 1;
	}
	else{
		fclose(fileTemp);
		return 0;
	}
}

unsigned FileHandle::getNumberOfPages()
{
	fseek(file, 0, SEEK_END);
	int size = ftell(file);
//	fseek(file, 0, SEEK_SET);

	return size / PAGE_SIZE;
//	return appendPageCounter;
}


RC FileHandle::collectCounterValues(unsigned &readPageCount, unsigned &writePageCount, unsigned &appendPageCount)
{
//	if(file == NULL)
//		return -1;
//	else{
		 readPageCount = this->readPageCounter;
		 writePageCount = this->writePageCounter;
		 appendPageCount = this->appendPageCounter;
		 return 0;
//	}
}
