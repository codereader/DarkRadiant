#ifndef CONSTANTS_H_
#define CONSTANTS_H_

// Possible patch types, should be Doom 3 anyway
enum EPatchType {
  ePatchTypeQuake3,
  ePatchTypeDoom3,
};

// Minimum height and width of a patch
#define MIN_PATCH_WIDTH 3
#define MIN_PATCH_HEIGHT 3

// greebo: Maximum patch width and height, Doom 3 has 99 as limit (at least this was defined with 99 in GtkRadiant) 
const std::size_t MAX_PATCH_WIDTH = 99;
const std::size_t MAX_PATCH_HEIGHT = 99;

#define MAX_PATCH_ROWCTRL (((MAX_PATCH_WIDTH-1)-1)/2)
#define MAX_PATCH_COLCTRL (((MAX_PATCH_HEIGHT-1)-1)/2)

// The cap types for a patch
enum EPatchCap {
  eCapBevel,
  eCapEndCap,
  eCapIBevel,
  eCapIEndCap,
  eCapCylinder,
};

// The pre-defined patch types
enum EPatchPrefab {
  ePlane,
  eBevel,
  eEndCap,
  eCylinder,
  eDenseCylinder,
  eVeryDenseCylinder,
  eSqCylinder,
  eCone,
  eSphere,
};

enum EMatrixMajor {
  ROW, COL,
};

#endif /*CONSTANTS_H_*/
