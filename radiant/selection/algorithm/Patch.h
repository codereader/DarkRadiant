#pragma once

#include "icommandsystem.h"

namespace selection
{

namespace algorithm
{

void invertPatch(const cmd::ArgumentList& args);

void redispersePatchRows(const cmd::ArgumentList& args);

void redispersePatchCols(const cmd::ArgumentList& args);

void transposePatch(const cmd::ArgumentList& args);

void capPatch(const cmd::ArgumentList& args);

void cyclePatchProjection(const cmd::ArgumentList& args);

void insertPatchColumnsAtEnd(const cmd::ArgumentList& args);
void insertPatchColumnsAtBeginning(const cmd::ArgumentList& args);
void insertPatchRowsAtEnd(const cmd::ArgumentList& args);
void insertPatchRowsAtBeginning(const cmd::ArgumentList& args);

void deletePatchColumnsFromBeginning(const cmd::ArgumentList& args);
void deletePatchColumnsFromEnd(const cmd::ArgumentList& args);
void deletePatchRowsFromBeginning(const cmd::ArgumentList& args);
void deletePatchRowsFromEnd(const cmd::ArgumentList& args);

void appendPatchColumnsAtBeginning(const cmd::ArgumentList& args);
void appendPatchColumnsAtEnd(const cmd::ArgumentList& args);
void appendPatchRowsAtBeginning(const cmd::ArgumentList& args);
void appendPatchRowsAtEnd(const cmd::ArgumentList& args);

/** 
 * greebo: This collects a list of all selected patches and thickens them
 * after querying the user for the thickness and the "createSeams" boolean.
 */
void thickenPatches(const cmd::ArgumentList& args);

} // namespace

} // namespace
