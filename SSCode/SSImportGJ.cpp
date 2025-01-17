// SSImportGJ.cpp
// SSCore
//
// Created by Tim DeBenedictis on 3/27/20.
// Copyright © 2020 Southern Stars. All rights reserved.

#include <algorithm>
#include <iostream>
#include <fstream>

#include "SSCoordinates.hpp"
#include "SSImportHIP.hpp"
#include "SSImportGJ.hpp"

// Comverts total proper motion (pm), position angle of motion (pa), and declination (dec)
// to proper motion in R.A. (pmra) and proper motion in Dec. (pmdec).  All angles in radians.

void pm_pa_to_pmra_pmdec ( double pm, double pa, double dec, double &pmra, double &pmdec )
{
    pmra = pm * sin ( pa ) / cos ( dec );
    pmdec = pm * cos ( pa );
}

// Comverts proper motion in R.A. (pmra) and proper motion in Dec. (pmdec), and declination (dec)
// to total proper motion (pm) and position angle of motion (pa). All angles in radians.
// TODO: NEEDS TESTING!

void pmra_pmdec_to_pm_pa ( double pmra, double pmdec, double dec, double &pm, double &pa )
{
    pmra *= cos ( dec );
    pm = sqrt ( pmra * pmra + pmdec * pmdec );
    pa = atan2pi ( pmra, pmdec );
}

// Adds a copy of a single GJ star (pStar) to a vector of SSObjects (stars).
// Provide the star's GJ identifier string (strGJ) WITHOUT prefix or components.
// The star's component letter (strC) should be a single-character or empty string.

void addGJStar ( SSStarPtr pStar, string strGJ, string strC, SSObjectVec &stars )
{
    SSStarPtr pNewStar = new SSStar ( *pStar );
    
    SSIdentifier identGJ = SSIdentifier::fromString ( "GJ " + strGJ + strC );
    pNewStar->addIdentifier ( identGJ );
    pNewStar->sortIdentifiers();
    
    stars.append ( pNewStar );
}

// Adds multiple components of a GJ star (pStar) to a vector of SSObjects (stars).
// Provide the star's GJ identifier string (strGJ) WITHOUT prefix or components.
// Adds one star for each character in the component string (comps).

int addGJComponentStars ( SSStarPtr pStar, string strGJ, string comps, SSObjectVec &stars )
{
    if ( comps.length() < 2 )
    {
        addGJStar ( pStar, strGJ, comps, stars );
        return 1;
    }
    else
    {
        for ( char c : comps )
            addGJStar ( pStar, strGJ, string ( 1, c ), stars );
        return (int) comps.length();
    }
}

// Imports Gliese-Jahreiss Catalog of Nearby Stars, 3rd (preliminary) Ed.:
// ftp://cdsarc.u-strasbg.fr/cats/V/70A/
// Imported stars are stored in the provided vector of SSObjects (stars).
// Names are added from nameMap, wherever possible.
// Accurate coordinates, proper motion, and HIP identifiers are added from hipStars.
// CNS lines representing multiple components are split into single components.
// Returns the total number of stars imported (should be 3849 if successful);
// original CNS3 contains 3803 lines; but multiples are split and Sun is excluded.

int SSImportGJCNS3 ( const string &filename, SSIdentifierNameMap &nameMap, SSObjectVec &gjACStars, SSObjectVec &stars )
{
    // Open file; return on failure.

    ifstream file ( filename );
    if ( ! file )
        return ( 0 );

    // Set up matrix for precessing B1950 coordinates and proper motion to J2000
    
    SSMatrix precession = SSCoordinates::getPrecessionMatrix ( SSTime::kB1950 ).transpose();
    
    // Read file line-by-line until we reach end-of-file

    string line = "";
    int numStars = 0;

    while ( getline ( file, line ) )
    {
        size_t len = line.length();
        if ( line.length() < 119 )
            continue;
        
        // Get GJ identifier and components (A, B, C, etc.)
        // Note we are ignoring the identifier prefix (GJ, Gl, NN, Wo)
        // and treating all identifiers as GJ numbers.
        
        string strGJ = trim ( line.substr ( 2, 6 ) );
         string comps = trim ( line.substr ( 8, 2 ) );

        // Get Identifier, HD, DM catalog numbers.

        string strHD = len < 153 ? "" : trim ( line.substr ( 146, 6 ) );
        string strDM = len < 165 ? "" : trim ( line.substr ( 153, 12 ) );

        // Extract RA and Dec. If either are blank, skip this line.
        
        string strRA = trim ( line.substr ( 12,8 ) );
        string strDec = trim ( line.substr ( 21, 8 ) );
        if ( strRA.empty() || strDec.empty() )
            continue;
        
        // Extract proper motion and position angle of proper motion
        
        string strPM = trim ( line.substr ( 30, 6 ) );
        string strPA = trim ( line.substr ( 37, 5 ) );
        
        // Extract radial velocity and spectral type.
        
        string strRV = trim ( line.substr ( 43, 6 ) );
        string strSpec = trim ( line.substr ( 54, 12 ) );

        // Extract Johnson V magnitude and B-V color index.
        
        string strVmag = trim ( line.substr ( 67, 6 ) );
        string strBmV = trim ( line.substr ( 76, 5 ) );

        // Extract resulting parallax and standard error of parallax.
        
        string strPlx = trim ( line.substr ( 108, 6 ) );
        string strPlxErr = trim ( line.substr ( 114, 5 ) );
        
        // Get B1950 Right Ascension and Declination
        
        double ra = degtorad ( strtodeg ( strRA ) * 15.0 );
        double dec = degtorad ( strtodeg ( strDec ) );
                
        // Get B1950 proper motion and position angle;
        // if both present convert to proper motion in R.A and Dec.

        double pmRA = INFINITY, pmDec = INFINITY;
        if ( ! strPM.empty() && ! strPA.empty() )
        {
            double pm = SSAngle::fromArcsec ( strtofloat64 ( strPM ) );
            double pa = SSAngle::fromDegrees ( strtofloat64 ( strPA ) );
            pm_pa_to_pmra_pmdec ( pm, pa, dec, pmRA, pmDec );
        }
        
        // Precess B1950 position and proper motion to J2000
        
        SSSpherical coords ( ra, dec, 1.0 );
        SSSpherical motion ( pmRA, pmDec, 0.0 );
        
        SSUpdateStarCoordsAndMotion ( 1950.0, &precession, coords, motion );

        // Get parallax in milliarcsec and convert to distance if > 1 mas.
        
        float plx = strtofloat ( strPlx );
        if ( plx > 1.0 )
            coords.rad = 1000.0 * SSCoordinates::kLYPerParsec / plx;
        
        // Get radial velocity in km/sec and convert to light speed.
        
        motion.rad = strRV.empty() ? INFINITY : strtofloat ( strRV ) / SSCoordinates::kLightKmPerSec;
        
        // Get Johnson V magnitude
        
        float vmag = INFINITY;
        if ( ! strVmag.empty() )
            vmag = strtofloat ( strVmag );
        
        // Get Johnson B magnitude from color index
        
        float bmag = INFINITY;
        if ( ! strBmV.empty() )
            bmag = strtofloat ( strBmV ) + vmag;

        // Set up identifier vector.  Parse HD, DM identifiers.
        // We'll add GJ identifier when adding components to star vector.

        vector<SSIdentifier> idents ( 0 );
        SSIdentifier identHD, identDM;
        
        if ( ! strHD.empty() )
            SSAddIdentifier ( SSIdentifier ( kCatHD, strtoint ( strHD ) ), idents );
        
        if ( ! strDM.empty() )
            SSAddIdentifier ( SSIdentifier::fromString ( strDM ), idents );

        // Attempt to parse variable-star designation.  Avoid strings that start with
        // "MU", "NU"; these are just capitalized Bayer letters, not legit GCVS idents.
        
        if ( len > 189 )
        {
            string strName = trim ( line.substr ( 188, string::npos ) );
            if ( strName.find ( "MU" ) == 0 || strName.find ( "NU" ) == 0 )
                strName = "";
            
            SSIdentifier ident = SSIdentifier::fromString ( strName );
            SSCatalog cat = ident.catalog();
            if ( cat == kCatGCVS )
                SSAddIdentifier ( ident, idents );
        }
        
        // Construct star and insert components into star vector.

        SSObjectPtr pObj = SSNewObject ( kTypeStar );
        SSStarPtr pStar = SSGetStarPtr ( pObj );
        if ( pStar == nullptr )
            continue;

        pStar->setIdentifiers ( idents );
        pStar->setFundamentalMotion ( coords, motion );
        pStar->setVMagnitude ( vmag );
        pStar->setBMagnitude ( bmag );
        pStar->setSpectralType ( strSpec );

        // cout << pStar->toCSV() << endl;
        numStars += addGJComponentStars ( pStar, strGJ, comps, stars );
    }
    
    // Set up GJ identifier mapping for retrieving accurate GJ coordinates and HIP identifiers.
    
    SSObjectMap map = SSMakeObjectMap ( gjACStars, kCatGJ );
    
    // For each component star in CNS3, find a GJ star with GJ accurate coordinates
    // and update original CNS3 star's coordinates, parallax, and identifiers.
    
    for ( int i = 0; i < stars.size(); i++ )
    {
        SSStarPtr pStar = SSGetStarPtr ( stars[i] );
        if ( pStar == nullptr )
            continue;
        
        SSIdentifierVec idents;
        SSIdentifier identGJ = pStar->getIdentifier ( kCatGJ );
        
        // Look up GJ star with accurate coordinates.  If we find one,
        // replace CNS3 coordinates and motion with accurate GJ coordinates, distance,
        // and proper motion (but not radial velocity!), and add HIP identifier.
        
        SSStarPtr pACStar = SSGetStarPtr ( SSIdentifierToObject ( identGJ, map, gjACStars ) );
        if ( pACStar != nullptr )
        {
            SSSpherical coords = pStar->getFundamentalCoords();
            SSSpherical motion = pStar->getFundamentalMotion();

            SSSpherical accCoords = pACStar->getFundamentalCoords();
            SSSpherical accMotion = pACStar->getFundamentalMotion();
            
            coords.lon = accCoords.lon;
            coords.lat = accCoords.lat;
            coords.rad = isinf ( accCoords.rad ) ? coords.rad : accCoords.rad;
            
            motion.lon = accMotion.lon;
            motion.lat = accMotion.lat;
            motion.rad = isinf ( accMotion.rad ) ? motion.rad : accMotion.rad;

            idents = pStar->getIdentifiers();
            
            SSAddIdentifier ( pACStar->getIdentifier ( kCatHIP ), idents );
            SSAddIdentifier ( pACStar->getIdentifier ( kCatBayer ), idents );
            SSAddIdentifier ( pACStar->getIdentifier ( kCatFlamsteed ), idents );
            SSAddIdentifier ( pACStar->getIdentifier ( kCatGCVS ), idents );
            
            sort ( idents.begin(), idents.end(), compareSSIdentifiers );
            pStar->setIdentifiers ( idents );
            pStar->setFundamentalMotion ( coords, motion );
        }
        
        // Finally add common names to individual stars
        
        vector<string> names = SSIdentifiersToNames ( idents, nameMap );
        if ( names.size() > 0 )
            pStar->setNames ( names );
    }
    
    // Return imported star count; file is closed automatically.

    return numStars;
}

// Imports Accurate Coordinates for Gliese Catalog Stars:
// https://cdsarc.unistra.fr/ftp/J/PASP/122/885
// Imported stars are stored in the provided vector of SSObjects (stars).
// Parallaxes, magnitudes, and identifiers are taken from Hipparcos stars (hipStars).
// Lines containing multiple components are split into individual single components.
// Returns the total number of stars imported (should be 4266 if successful);
// original file contains 4106 lines, but multiples are split into single components.

int SSImportGJAC ( const string &filename, SSObjectVec &hipStars, SSObjectVec &stars )
{
    // Open file; return on failure.

    ifstream file ( filename );
    if ( ! file )
        return ( 0 );

    // Set up HIP identifier mapping for retrieving Hipparcos stars.
    
    SSObjectMap map = SSMakeObjectMap ( hipStars, kCatHIP );
    
    // Read file line-by-line until we reach end-of-file

    string line = "";
    int numStars = 0;

    while ( getline ( file, line ) )
    {
        if ( line.length() < 124 )
            continue;
        
        // Get Gl/GJ/NN/Wo Identifier (including component A, B, C, etc.)
        // Get HIP or other identifier.
        
        string strGJ = trim ( line.substr ( 2, 20 ) );
        string strHIP = trim ( line.substr ( 22, 13 ) );
        
        // Extract components from GJ identifier, then erase from identifier
        
        string comps = "";
        size_t pos1 = strGJ.find_first_of ( "ABCD" );
        if ( pos1 != string::npos )
        {
            // A few identifiers are duplicates on a single line separated
            // by a slash (example: GJ 3406 A/3407 B); ignore the duplicate.
            
            size_t pos2 = strGJ.find_first_of ( "/" );
            comps = trim ( strGJ.substr ( pos1 - 1, pos2 == string::npos ? string::npos : pos2 - pos1 ) );
            strGJ.erase ( pos1, string::npos );
        }

        // Extract RA and Dec. If either are blank, skip this line.
        
        string strRA = trim ( line.substr ( 36, 11 ) );
        string strDec = trim ( line.substr ( 48, 11 ) );
        if ( strRA.empty() || strDec.empty() )
            continue;
        
        // Extract proper motion in R.A. and Dec.
        
        string strPMRA = trim ( line.substr ( 61, 6 ) );
        string strPMDec = trim ( line.substr ( 69, 6 ) );
        
        // Extract 2MASS J and H magnitudes
        
        string strJmag = trim ( line.substr ( 94, 6 ) );
        string strHmag = trim ( line.substr ( 101, 6 ) );

        // Get J2000 Right Ascension and Declination
        
        double ra = degtorad ( strtodeg ( strRA ) * 15.0 );
        double dec = degtorad ( strtodeg ( strDec ) );
        
        // Convert J2000 proper motion from arcsec to radians
        
        float pmRA = INFINITY;
        if ( ! strPMRA.empty() )
            pmRA = SSAngle::fromArcsec ( strtofloat ( strPMRA ) ) / cos ( dec );
        
        float pmDec = INFINITY;
        if ( ! strPMDec.empty() )
            pmDec = SSAngle::fromArcsec ( strtofloat ( strPMDec ) );

        SSSpherical coords ( ra, dec, INFINITY );
        SSSpherical motion ( pmRA, pmDec, INFINITY );

        // Get 2MASS J and H magnitudes.  Ignored for now.
        
        float vmag = INFINITY; // strJmag.empty() ? INFINITY : strtofloat ( strJmag );
        float bmag = INFINITY; // strHmag.empty() ? INFINITY : strtofloat ( strHmag );

        // Set up name and identifier vectors.

        vector<SSIdentifier> idents ( 0 );
        vector<string> names ( 0 );

        SSIdentifier hipID = SSIdentifier::fromString ( strHIP );
        if ( hipID )
            SSAddIdentifier ( hipID, idents );

        // Look up Hipparcos star from HIP identifier.  If we find one,
        // add distance, magnitudes, and selected identifiers.
        
        SSStarPtr pHIPStar = SSGetStarPtr ( SSIdentifierToObject ( hipID, map, hipStars ) );
        if ( pHIPStar != nullptr )
        {
            coords.rad = SSCoordinates::kLYPerParsec / pHIPStar->getParallax();
            motion.rad = pHIPStar->getRadVel();

            vmag = pHIPStar->getVMagnitude();
            bmag = pHIPStar->getBMagnitude();

            SSAddIdentifier ( pHIPStar->getIdentifier ( kCatBayer ), idents );
            SSAddIdentifier ( pHIPStar->getIdentifier ( kCatFlamsteed ), idents );
            SSAddIdentifier ( pHIPStar->getIdentifier ( kCatGCVS ), idents );
        }

       // Construct star and insert componenets into star vector.

        SSObjectPtr pObj = SSNewObject ( kTypeStar );
        SSStarPtr pStar = SSGetStarPtr ( pObj );
        if ( pStar == nullptr )
            continue;
        
        pStar->setNames ( names );
        pStar->setIdentifiers ( idents );
        pStar->setFundamentalMotion ( coords, motion );
        pStar->setVMagnitude ( vmag );
        pStar->setBMagnitude ( bmag );

        // cout << pStar->toCSV() << endl;
        numStars += addGJComponentStars ( pStar, strGJ, comps, stars );
    }
    
    // Return imported star count; file is closed automatically.

    return numStars;
}
