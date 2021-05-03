// SSImportWDS.hpp
// SSCore
//
// Created by Tim DeBenedictis on 4/26/21.
// Copyright © 2021 Southern Stars Group, LLC. All rights reserved.
//
// Functions for importing data from the Washington Double Star catalog
// and the Sixth Catalog of Orbits of Visual Binary Stars.

#ifndef SSImportWDS_hpp
#define SSImportWDS_hpp

#include "SSStar.hpp"

int SSImportORB6  ( const string &filename, SSObjectArray &stars );
int SSImportWDS  ( const string &filename, const SSIdentifierMap &identmap, SSObjectArray &stars );
int SSImportWDSHIPCrossIndex ( const string &filename, SSIdentifierMap &identmap );
int SSImportWDSBFCrossIndex ( const string &filename, SSIdentifierMap &identmap );

#endif /* SSImportWDS_hpp */