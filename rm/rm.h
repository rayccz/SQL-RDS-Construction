
#ifndef _rm_h_
#define _rm_h_

#include <string>
#include <vector>

#include "../rbf/rbfm.h"
#include "../ix/ix.h"

using namespace std;

# define RM_EOF (-1)  // end of a scan operator

// RM_ScanIterator is an iterator to go through tuples
class RM_ScanIterator {
public:
  RM_ScanIterator() {};
  ~RM_ScanIterator() {};

  // "data" follows the same format as RelationManager::insertTuple()

  string tableName;
  string conditionAttribute;
  CompOp compOp = NO_OP;
  void *value = NULL;
  vector<string> attributeNames;
  FileHandle fileHandle;
  vector<Attribute> recordDescriptor;
  RBFM_ScanIterator rbfm_ScanIterator;

  RC getNextTuple(RID &rid, void *data) {
	  if (rbfm_ScanIterator.getNextRecord(rid, data) == -1){
		  return RM_EOF;
	  }else{
		  return 0;
	  }
  };
  RC close() {
	  RC rc = RecordBasedFileManager::instance()->closeFile(fileHandle);
	  return rc;
	  //return -1;
  };
};


// RM_IndexScanIterator is an iterator to go through index entries
class RM_IndexScanIterator {
 public:

  RM_IndexScanIterator() {};  	// Constructor
  ~RM_IndexScanIterator() {}; 	// Destructor


  // "key" follows the same format as in IndexManager::insertEntry()
  RC getNextEntry(RID &rid, void *key) {// Get next matching entry
	  return ix_scanIterator.getNextEntry(rid, key);
  };

  RC close() {// Terminate index scan
	  return ix_scanIterator.close();
  };

  IX_ScanIterator ix_scanIterator;
//  IXFileHandle ixfileHandle;
//  Attribute ixAttr;
};


// Relation Manager
class RelationManager
{
public:
  static RelationManager* instance();

  int maxtableid=0;													  // Record the max value of table id

  RC createCatalog();

  RC deleteCatalog();

  RC createTable(const string &tableName, const vector<Attribute> &attrs);

  RC deleteTable(const string &tableName);

  RC getAttributes(const string &tableName, vector<Attribute> &attrs);

  RC insertTuple(const string &tableName, const void *data, RID &rid);

  RC deleteTuple(const string &tableName, const RID &rid);

  RC updateTuple(const string &tableName, const void *data, const RID &rid);

  RC readTuple(const string &tableName, const RID &rid, void *data);

  // Print a tuple that is passed to this utility method.
  // The format is the same as printRecord().
  RC printTuple(const vector<Attribute> &attrs, const void *data);

  RC readAttribute(const string &tableName, const RID &rid, const string &attributeName, void *data);

  // Scan returns an iterator to allow the caller to go through the results one by one.
  // Do not store entire results in the scan iterator.
  RC scan(const string &tableName,
      const string &conditionAttribute,
      const CompOp compOp,                  // comparison type such as "<" and "="
      const void *value,                    // used in the comparison
      const vector<string> &attributeNames, // a list of projected attributes
      RM_ScanIterator &rm_ScanIterator);


  RC createIndex(const string &tableName, const string &attributeName);

  RC destroyIndex(const string &tableName, const string &attributeName);

  // indexScan returns an iterator to allow the caller to go through qualified entries in index
  RC indexScan(const string &tableName,
                        const string &attributeName,
                        const void *lowKey,
                        const void *highKey,
                        bool lowKeyInclusive,
                        bool highKeyInclusive,
                        RM_IndexScanIterator &rm_IndexScanIterator);

// Extra credit work (10 points)
public:
  RC addAttribute(const string &tableName, const Attribute &attr);

  RC dropAttribute(const string &tableName, const string &attributeName);


protected:
  RelationManager();
  ~RelationManager();

};

#endif
