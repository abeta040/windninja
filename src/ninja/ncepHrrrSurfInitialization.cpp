/******************************************************************************
*
* $Id: ncepNamSurfInitialization.cpp 1757 2012-08-07 18:40:40Z kyle.shannon $
*
* Project:  WindNinja
* Purpose:  Initializing with NCEP HRRR forecsasts in GRIB2 format
* Author:   Natalie Wagenbrenner <nwagenbrenner@gmail.com>
*
******************************************************************************
*
* THIS SOFTWARE WAS DEVELOPED AT THE ROCKY MOUNTAIN RESEARCH STATION (RMRS)
* MISSOULA FIRE SCIENCES LABORATORY BY EMPLOYEES OF THE FEDERAL GOVERNMENT
* IN THE COURSE OF THEIR OFFICIAL DUTIES. PURSUANT TO TITLE 17 SECTION 105
* OF THE UNITED STATES CODE, THIS SOFTWARE IS NOT SUBJECT TO COPYRIGHT
* PROTECTION AND IS IN THE PUBLIC DOMAIN. RMRS MISSOULA FIRE SCIENCES
* LABORATORY ASSUMES NO RESPONSIBILITY WHATSOEVER FOR ITS USE BY OTHER
* PARTIES,  AND MAKES NO GUARANTEES, EXPRESSED OR IMPLIED, ABOUT ITS QUALITY,
* RELIABILITY, OR ANY OTHER CHARACTERISTIC.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*
*****************************************************************************/

#include "ncepHrrrSurfInitialization.h"

/**
* Constructor for the class initializes the variable names
*
*/
ncepHrrrSurfInitialization::ncepHrrrSurfInitialization() : wxModelInitialization()
{

}

/**
* Copy constructor.
* @param A Copied value.
*/
ncepHrrrSurfInitialization::ncepHrrrSurfInitialization(ncepHrrrSurfInitialization const& A ) : wxModelInitialization(A)
{
    wxModelFileName = A.wxModelFileName;
}

/**
* Destructor.
*/
ncepHrrrSurfInitialization::~ncepHrrrSurfInitialization()
{
}

/**
* Equals operator.
* @param A Value to set equal to.
* @return a copy of an object
*/
ncepHrrrSurfInitialization& ncepHrrrSurfInitialization::operator= (ncepHrrrSurfInitialization const& A)
{
    if(&A != this) {
        wxModelInitialization::operator=(A);
    }
    return *this;
}

/**
*@brief wrapper for GetWindHeight
*@return return wind height
*/
double ncepHrrrSurfInitialization::Get_Wind_Height()
{
    //can grab this directly from grib file later
    return 10;
}

/**
* Fetch the variable names
*
* @return a vector of variable names
*/
std::vector<std::string> ncepHrrrSurfInitialization::getVariableList()
{
    std::vector<std::string> varList;
    varList.push_back( "2t" );
    varList.push_back( "10v" );
    varList.push_back( "10u" );
    varList.push_back( "gh" ); // Geopotential cloud top height
    return varList;
}

/**
* Fetch a unique identifier for the forecast.  This string can be used for
* identifying and storing.
*
* @return unique name
*/
std::string ncepHrrrSurfInitialization::getForecastIdentifier()
{
    return std::string( "NCEP-HRRR-3km-SURFACE" );
}

/**
* Fetch the path to the forecast
*
* @return path to the forecast
*/
std::string ncepHrrrSurfInitialization::getPath()
{
    return std::string( "" );
}

int ncepHrrrSurfInitialization::getStartHour()
{
    return 0;
}

int ncepHrrrSurfInitialization::getEndHour()
{
    return 84;
}

/**
* Checks the downloaded data to see if it is all valid.
*/
void ncepHrrrSurfInitialization::checkForValidData()
{

}

/**
* Static identifier to determine if the file is an HRRR forecast.
* If don't have access to grib api, identificaion is done based
* on filename and id of 10u band.
* @param fileName grib2 filename
*
* @return true if the forecast is a NCEP HRRR forecast
*/

bool ncepHrrrSurfInitialization::identify( std::string fileName )
{

    bool identified = true;

    if( fileName.find("nam") != fileName.npos ) {
        identified = false;
        return identified;
    }

    //ID based on 10u band
    GDALDataset *srcDS;
    srcDS = (GDALDataset*)GDALOpenShared( fileName.c_str(), GA_ReadOnly );

    if( srcDS == NULL ) {
        CPLDebug( "ncepHRRRSurfaceInitialization::identify()",
                "Bad forecast file" );
        return false;
    }

    if( srcDS->GetRasterCount() < 8 )
    {
        /* Short circuit */
        GDALClose( (GDALDatasetH)srcDS );
        identified = false;
        return identified;
    }
    GDALRasterBand *poBand = srcDS->GetRasterBand( 33 ); //2010 structure
    const char *gc;
    gc = poBand->GetMetadataItem( "GRIB_COMMENT" );
    std::string bandName( gc );

    if( bandName.find( "u-component of wind [m/s]" ) == bandName.npos ){
        poBand = srcDS->GetRasterBand( 49 ); //files after 2010 have different structure
        gc = poBand->GetMetadataItem( "GRIB_COMMENT" );
        bandName = gc;
        if( bandName.find( "u-component of wind [m/s]" ) == bandName.npos ){
            poBand = srcDS->GetRasterBand( 50 ); //2012 files have different structure
            gc = poBand->GetMetadataItem( "GRIB_COMMENT" );
            bandName = gc;
            if( bandName.find( "u-component of wind [m/s]" ) == bandName.npos ){
                identified = false;
            }
        }

    }
    GDALClose( (GDALDatasetH)srcDS );

    return identified;

}

/**
* Sets the surface grids based on a ncep HRRR (surface only!) forecast.
* @param input The WindNinjaInputs for misc. info.
* @param airGrid The air temperature grid to be filled.
* @param cloudGrid The cloud cover grid to be filled.
* @param uGrid The u velocity grid to be filled.
* @param vGrid The v velocity grid to be filled.
* @param wGrid The w velocity grid to be filled (filled with zeros here?).
*/
void ncepHrrrSurfInitialization::setSurfaceGrids( WindNinjaInputs &input,
        AsciiGrid<double> &airGrid,
        AsciiGrid<double> &cloudGrid,
        AsciiGrid<double> &uGrid,
        AsciiGrid<double> &vGrid,
        AsciiGrid<double> &wGrid )
{
    int bandNum = -1;

    GDALDataset *srcDS;
    srcDS = (GDALDataset*)GDALOpenShared( input.forecastFilename.c_str(), GA_ReadOnly );

    if( srcDS == NULL ) {
        CPLDebug( "ncepHRRRSurfaceInitialization::identify()",
                "Bad forecast file" );
    }

    GDALRasterBand *poBand = srcDS->GetRasterBand( 49 );
    const char *gc;
    gc = poBand->GetMetadataItem( "GRIB_COMMENT" );
    std::string bandName( gc );

    //get time list
    std::vector<boost::local_time::local_date_time> timeList( getTimeList( input.ninjaTimeZone ) );

    //Search time list for our time to identify our band number for cloud/speed/dir
    //Right now, just one time step per file
    std::vector<int> bandList;
    for(unsigned int i = 0; i < timeList.size(); i++)
    {
        if(input.ninjaTime == timeList[i])
        {
            //check which HRRR format we have
            if( bandName.find( "u-component of wind [m/s]" ) == bandName.npos ){ //if band 49 isn't u10, it's either 2010 or 2012 format
                GDALRasterBand *poBand = srcDS->GetRasterBand( 50 );
                const char *gc;
                gc = poBand->GetMetadataItem( "GRIB_COMMENT" );
                std::string bandName( gc );
                if( bandName.find( "u-component of wind [m/s]" ) == bandName.npos ){ //if band 50 isn't u10, it's the 2010 format
                    bandList.push_back( 29 ); // 2t
                    bandList.push_back( 34 ); // 10v
                    bandList.push_back( 33 );  // 10u
                    bandList.push_back( 52 ); // geopotential height at cloud top
                }
                else{
                    bandList.push_back( 45 ); // 2t
                    bandList.push_back( 51 ); // 10v
                    bandList.push_back( 50 );  // 10u
                    bandList.push_back( 78 ); // geopotential height at cloud top
                }
            }
            else{ //otherwise, should be 2011 format, but check for u10 band to be sure
                poBand = srcDS->GetRasterBand( 44 );
                gc = poBand->GetMetadataItem( "GRIB_COMMENT" );
                bandName = gc;
                if( bandName.find( "u-component of wind [m/s]" ) == bandName.npos ){
                    CPLDebug( "ncepHRRRSurfaceInitialization::identify()",
                              "Can't find the u-10 band in the forecast file." );
                }
                bandList.push_back( 44 ); // 2t
                bandList.push_back( 50 ); // 10v
                bandList.push_back( 49 );  // 10u
                bandList.push_back( 73 ); // geopotential height at cloud top
            }
            break;
        }
    }

    if(bandList.size() < 4)
        throw std::runtime_error("Could not match ninjaTime with a band number in the forecast file.");

    std::string dstWkt;
    dstWkt = input.dem.prjString;

    GDALDataset *wrpDS;
    std::string temp;
    std::string srcWkt;

    GDALWarpOptions* psWarpOptions;

    srcWkt = srcDS->GetProjectionRef();

    poBand = srcDS->GetRasterBand( 9 );
    int pbSuccess;
    double dfNoData = poBand->GetNoDataValue( &pbSuccess );

    psWarpOptions = GDALCreateWarpOptions();

    int nBandCount = bandList.size();

    psWarpOptions->nBandCount = nBandCount;
    psWarpOptions->panSrcBands =
        (int*) CPLMalloc( sizeof( int ) * nBandCount );
    psWarpOptions->panDstBands =
        (int*) CPLMalloc( sizeof( int ) * nBandCount );
    psWarpOptions->padfDstNoDataReal =
        (double*) CPLMalloc( sizeof( double ) * nBandCount );
    psWarpOptions->padfDstNoDataImag =
        (double*) CPLMalloc( sizeof( double ) * nBandCount );


    psWarpOptions->padfDstNoDataReal =
        (double*) CPLMalloc( sizeof( double ) * nBandCount );
    psWarpOptions->padfDstNoDataImag =
        (double*) CPLMalloc( sizeof( double ) * nBandCount );

    if( pbSuccess == false )
        dfNoData = -9999.0;

    psWarpOptions->panSrcBands =
        (int *) CPLMalloc(sizeof(int) * psWarpOptions->nBandCount );
    psWarpOptions->panSrcBands[0] = bandList[0];
    psWarpOptions->panSrcBands[1] = bandList[1];
    psWarpOptions->panSrcBands[2] = bandList[2];
    psWarpOptions->panSrcBands[3] = bandList[3];

    psWarpOptions->panDstBands =
        (int *) CPLMalloc(sizeof(int) * psWarpOptions->nBandCount );
    psWarpOptions->panDstBands[0] = 1;
    psWarpOptions->panDstBands[1] = 2;
    psWarpOptions->panDstBands[2] = 3;
    psWarpOptions->panDstBands[3] = 4;

    wrpDS = (GDALDataset*) GDALAutoCreateWarpedVRT( srcDS, srcWkt.c_str(),
                                                    dstWkt.c_str(),
                                                    GRA_NearestNeighbour,
                                                    1.0, psWarpOptions );
    std::vector<std::string> varList = getVariableList();

    for( unsigned int i = 0; i < varList.size(); i++ ) {
        if( varList[i] == "2t" ) {
            GDAL2AsciiGrid( wrpDS, i+1, airGrid );
            if( CPLIsNan( dfNoData ) ) {
                airGrid.set_noDataValue( -9999.0 );
                airGrid.replaceNan( -9999.0 );
            }
        }
        else if( varList[i] == "10v" ) {
            GDAL2AsciiGrid( wrpDS, i+1, vGrid );
            if( CPLIsNan( dfNoData ) ) {
                vGrid.set_noDataValue( -9999.0 );
                vGrid.replaceNan( -9999.0 );
            }
        }
        else if( varList[i] == "10u" ) {
            GDAL2AsciiGrid( wrpDS, i+1, uGrid );
            if( CPLIsNan( dfNoData ) ) {
                uGrid.set_noDataValue( -9999.0 );
                uGrid.replaceNan( -9999.0 );
            }
        }
        else if( varList[i] == "gh" ) {
            GDAL2AsciiGrid( wrpDS, i+1, cloudGrid );
            if( CPLIsNan( dfNoData ) ) {
                cloudGrid.set_noDataValue( -9999.0 );
                cloudGrid.replaceNan( -9999.0 );
            }
        }
    }
    //if there are any clouds set cloud fraction to 1, otherwise set to 0.
    for(int i = 0; i < cloudGrid.get_nRows(); i++){
        for(int j = 0; j < cloudGrid.get_nCols(); j++){
            if(cloudGrid(i,j) < 0.0){
                cloudGrid(i,j) = 0.0;
            }
            else{
                cloudGrid(i,j) = 1.0;
            }
        }
    }
    wGrid.set_headerData( uGrid );
    wGrid = 0.0;
    airGrid += 273.15;

    GDALDestroyWarpOptions( psWarpOptions );
    GDALClose((GDALDatasetH) srcDS );
    GDALClose((GDALDatasetH) wrpDS );
}
