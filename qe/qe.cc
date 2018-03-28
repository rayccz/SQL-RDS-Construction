
#include "qe.h"

Filter::Filter(Iterator* input, const Condition &condition) {
	this->input = input;
	this->condition = condition;
	input->getAttributes(this->attrs);
	leftValue = malloc(PAGE_SIZE);
	rightValue = malloc(PAGE_SIZE);
}

// ... the rest of your implementations go here


Project::Project(Iterator *input,			// Iterator of input R
		const vector<string> &attrNames){	//vector containing attribute names
	this->input = input;
	this->needAttrNames = attrNames;
	input->getAttributes(allAttrs);
	getAttributes(this->needAttrs);
	recordData = malloc(PAGE_SIZE);
}

BNLJoin::BNLJoin(Iterator *leftIn,            // Iterator of input R
               TableScan *rightIn,           // TableScan Iterator of input S
               const Condition &condition,   // Join condition
               const unsigned numPages       // # of pages that can be loaded into memory,
			                                 //   i.e., memory block size (decided by the optimizer)
        ){
	this->leftIn = leftIn;
	this->rightIn = rightIn;
	this->condition = condition;
	this->numPages = numPages;
	leftIn->getAttributes(leftAttrs);
	rightIn->getAttributes(rightAttrs);
	leftValue = malloc(PAGE_SIZE);
	rightValue = malloc(PAGE_SIZE);
	leftRecord = malloc(PAGE_SIZE);
	rightRecord = malloc(PAGE_SIZE);
	isAfterFirstTimeLeft = false;
}



INLJoin::INLJoin(Iterator *leftIn,           // Iterator of input R
               IndexScan *rightIn,          // IndexScan Iterator of input S
               const Condition &condition   // Join condition
        ){
	this->condition = condition;
	this->leftIn = leftIn;
	this->rightIn = rightIn;
	this->leftRecord = malloc(PAGE_SIZE);
	this->leftValue = malloc(PAGE_SIZE);
	this->rightRecord = malloc(PAGE_SIZE);
	this->rightValue = malloc(PAGE_SIZE);
	this->isAfterFirstTimeLeft = false;
	leftIn->getAttributes(leftAttrs);
	rightIn->getAttributes(rightAttrs);
};






