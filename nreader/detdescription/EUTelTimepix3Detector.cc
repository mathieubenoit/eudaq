
// personal includes ".h"
#include "EUTELESCOPE.h"
#include "EUTelTimepix3Detector.h"

// system includes <>
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>

using namespace std;
using namespace eutelescope;

EUTelTimepix3Detector::EUTelTimepix3Detector() : EUTelPixelDetector()  {

/* For S3B system */
/*
  _xMin = 0;
  _xMax = 63;

  _yMin = 0;
  _yMax = 255;

  _xPitch = 0.024;
  _yPitch = 0.024;

*/

/* for DCD */
/*
  _xMin = 0;
  _xMax = 127;
  _yMin = 0;
  _yMax = 15;


*/

/* for DCD matrix */
/*  _xMin = 0;
  _xMax = 63;
  _yMin = 0;
  _yMax = 31;
*/
/* for DCD 4-fold matrix */
  _xMin = 0;
  _xMax = 255;
  _yMin = 0;
  _yMax = 255;


  _xPitch = 0.055;
  _yPitch = 0.055;


  _name = "Timepix3";


}


void EUTelTimepix3Detector::setMode( string mode ) {

  _mode = mode;

}


bool EUTelTimepix3Detector::hasSubChannels() const {
  if (  _subChannelsWithoutMarkers.size() != 0 ) return true;
  else return false;
}

std::vector< EUTelROI > EUTelTimepix3Detector::getSubChannels( bool withMarker ) const {

  if ( withMarker ) return _subChannelsWithMarkers;
  else  return _subChannelsWithoutMarkers;

}

EUTelROI EUTelTimepix3Detector::getSubChannelBoundary( size_t iChan, bool withMarker ) const {
  if ( withMarker ) return _subChannelsWithMarkers.at( iChan );
  else return _subChannelsWithoutMarkers.at( iChan );

}


void EUTelTimepix3Detector::print( ostream& os ) const {

  size_t w = 35;


  os << resetiosflags(ios::right)
     << setiosflags(ios::left)
     << setfill('.') << setw( w ) << setiosflags(ios::left) << "Detector name " << resetiosflags(ios::left) << " " << _name << endl
     << setw( w ) << setiosflags(ios::left) << "Mode " << resetiosflags(ios::left) << " " << _mode << endl
     << setw( w ) << setiosflags(ios::left) << "Pixel along x " << resetiosflags(ios::left) << " from " << _xMin << " to " << _xMax << endl
     << setw( w ) << setiosflags(ios::left) << "Pixel along y " << resetiosflags(ios::left) << " from " << _yMin << " to " << _yMax << endl
     << setw( w ) << setiosflags(ios::left) << "Pixel pitch along x " << resetiosflags(ios::left) << " " << _xPitch << "  mm  "  << endl
     << setw( w ) << setiosflags(ios::left) << "Pixel pitch along y " << resetiosflags(ios::left) << " " << _yPitch << "  mm  "  << endl;




}
