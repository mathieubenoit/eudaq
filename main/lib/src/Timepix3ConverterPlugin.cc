#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/PluginManager.hh"
#include "eudaq/Utils.hh"
#include <stdio.h>
#include <string.h>
#include <vector>
#include <iostream>
#include <ostream>

// All LCIO-specific parts are put in conditional compilation blocks
// so that the other parts may still be used if LCIO is not available.
#if USE_LCIO
#  include "IMPL/LCEventImpl.h"
#  include "IMPL/TrackerRawDataImpl.h"
#  include "IMPL/TrackerDataImpl.h"
#  include "IMPL/LCCollectionVec.h"
#  include "UTIL/CellIDEncoder.h"
#  include "lcio.h"
#endif

#if USE_EUTELESCOPE
#  include "EUTELESCOPE.h"
#  include "EUTelRunHeaderImpl.h"
#  include "EUTelTimepix3Detector.h"
#  include "EUTelSetupDescription.h"
#  include "EUTelEventImpl.h"
#  include "EUTelTrackerDataInterfacerImpl.h"
#  include "EUTelGenericSparsePixel.h"
using eutelescope::EUTELESCOPE;
#endif
#define MATRIX_SIZE 65536




using namespace std;

namespace eudaq {

  // The event type for which this converter plugin will be registered
  // Modify this to match your actual event type (from the Producer)
	static const char* EVENT_TYPE = "Timepix3Raw";
  
  // Declare a new class that inherits from DataConverterPlugin
  class Timepix3ConverterPlugin : public DataConverterPlugin {
    
  public:
#ifdef WRITE_TOT
    FILE *totfile;
#endif
    // This is called once at the beginning of each run.
    // You may extract information from the BORE and/or configuration
    // and store it in member variables to use during the decoding later.
    virtual void Initialize(const Event & bore,
			    const Configuration & cnf) {
      m_XMLConfig = bore.GetTag("XMLConfig", "");
#ifndef WIN32  //some linux Stuff //$$change
      (void)cnf; // just to suppress a warning about unused parameter cnf
#endif
#ifdef WRITE_TOT
      totfile=fopen("tot.txt","w");
#endif
    }
    
    // This should return the trigger ID (as provided by the TLU)
    // if it was read out, otherwise it can either return (unsigned)-1,
    // or be left undefined as there is already a default version.
    virtual unsigned GetTriggerID(const Event & ev) const {
      // Make sure the event is of class RawDataEvent
      if (const RawDataEvent * rev = dynamic_cast<const RawDataEvent *> (&ev)) {

	if ( rev->NumBlocks() > 0 && rev->GetBlock(0).size() >= sizeof(short) ) {

	  std::vector<unsigned char> data = rev->GetBlock( 0 ); // block 0 is trigger data
	  
	  // Note: here data has always exactly 12 elements
	  // [0->7]: TLU trigger timestamp
	  // [8->9]: TLU trigger number
	  // [10->11] SPIDR internal trigger number

	  return ( data[8] | data[9] );
	}
      }
      // If we are unable to extract the Trigger ID, signal with (unsigned)-1
      return (unsigned)-1;
    }
    
    // Here, the data from the RawDataEvent is extracted into a StandardEvent.
    // The return value indicates whether the conversion was successful.
    // Again, this is just an example, adapted it for the actual data layout.
    virtual bool GetStandardSubEvent(StandardEvent & sev, const Event & ev) const {
      // If the event type is used for different sensors
      // they can be differentiated here
      std::string sensortype = "timepix3";
            
      // Unpack data
      const RawDataEvent * rev = dynamic_cast<const RawDataEvent *> ( &ev );
      std::cout << "[Number of blocks] " << rev->NumBlocks() << std::endl;
      std::vector<unsigned char> data = rev->GetBlock( 1 ); // block 1 is pixel data
      std::cout << "vector has size : " << data.size() << std::endl;

      // Create a StandardPlane representing one sensor plane
      int id = 6;
      StandardPlane plane(id, EVENT_TYPE, sensortype);
      
      // Size of one pixel data chunk: 12 bytes = 1+1+2+8 bytes for x,y,tot,ts
      const unsigned int PIX_SIZE = 12;
      
      // Set the number of pixels
      int width = 256, height = 256;
      plane.SetSizeZS( width, height, ( data.size() ) / PIX_SIZE );
      
      std::vector<unsigned char> ZSDataX;
      std::vector<unsigned char> ZSDataY;
      std::vector<unsigned short> ZSDataTOT;
      std::vector<uint64_t> ZSDataTS;      
      size_t offset = 0;
      unsigned char aWord = 0;
      
      for( unsigned int i = 0; i < ( data.size() ) / PIX_SIZE; i++ ) {

	ZSDataX   .push_back( unpackXorY( data, offset + sizeof( aWord ) * 0 ) );
	ZSDataY   .push_back( unpackXorY( data, offset + sizeof( aWord ) * 1 ) );	
	ZSDataTOT .push_back( unpackTOT(  data, offset + sizeof( aWord ) * 2 ) );
	ZSDataTS  .push_back( unpackTS(   data, offset + sizeof( aWord ) * 4 ) );

	offset += sizeof( aWord ) * PIX_SIZE; 

	//std::cout << "[DATA] "  << " " << (int)ZSDataX[i] << " " << (int)ZSDataY[i] << " " << ZSDataTOT[i] << " " << ZSDataTS[i] << std::endl;

      }

      // Set the trigger ID
      plane.SetTLUEvent( GetTriggerID(ev) );
      
      for( size_t i = 0 ; i < ZSDataX.size(); ++i ) {
	plane.SetPixel(i, ZSDataX[i], ZSDataY[i], ZSDataTOT[i] );
      }

      // Add the plane to the StandardEvent
      sev.AddPlane( plane );
      
      // Indicate that data was successfully converted
      return true;
    }

    unsigned char unpackXorY( std::vector<unsigned char> data, size_t offset ) const {
      return data[offset];
    }
 
    unsigned short unpackTOT(  std::vector<unsigned char> data, size_t offset ) const {
      unsigned short tot = 0;
      for( unsigned int j = 0; j < 2; j++ ) {
	tot = tot | ( data[offset+j] << j*8 );
      }
      return tot;
    }

    uint64_t unpackTS( std::vector<unsigned char> data, size_t offset ) const {
      uint64_t ts = 0; 
      for( unsigned int j = 0; j < 8; j++ ) {
	ts = ts | ( data[offset+j] << j*8 );
      }
      return ts;
    }
    
#if USE_LCIO && USE_EUTELESCOPE

    void ConvertLCIOHeader(lcio::LCRunHeader & header, eudaq::Event const & /*bore*/, eudaq::Configuration const & /*conf*/) const {
      eutelescope::EUTelRunHeaderImpl runHeader(&header);
    }

    bool GetLCIOSubEvent(lcio::LCEvent & result, const Event & source) const {
      //Unused variable:
      //TrackerRawDataImpl *rawMatrix;
      TrackerDataImpl *zsFrame;

      if (source.IsBORE()) {
	// shouldn't happen
	return true;
      } else if (source.IsEORE()) {
	// nothing to do
	return true;
      }
      // If we get here it must be a data event

      // prepare the collections for the rawdata and the zs ones
      //auto_ptr< lcio::LCCollectionVec > rawDataCollection ( new lcio::LCCollectionVec (lcio::LCIO::TRACKERRAWDATA) ) ;
      auto_ptr< lcio::LCCollectionVec > zsDataCollection ( new lcio::LCCollectionVec (lcio::LCIO::TRACKERDATA) ) ;

      // set the proper cell encoder
      //CellIDEncoder< TrackerRawDataImpl > rawDataEncoder ( eutelescope::EUTELESCOPE::MATRIXDEFAULTENCODING, rawDataCollection.get() );
      CellIDEncoder< TrackerDataImpl > zsDataEncoder ( eutelescope::EUTELESCOPE::ZSDATADEFAULTENCODING, zsDataCollection.get() );

      // a description of the setup
      std::vector< eutelescope::EUTelSetupDescription * >  setupDescription;
      // FIXME hardcoded number of planes
      size_t numplanes = 1;
      std::string  mode;

      for (size_t iPlane = 0; iPlane < numplanes; ++iPlane) {

          std::string sensortype = "timepix3";

          // Unpack data
          const RawDataEvent * rev = dynamic_cast<const RawDataEvent *> ( &source );
          //std::cout << "[Number of blocks] " << rev->NumBlocks() << std::endl;
          std::vector<unsigned char> data = rev->GetBlock( 1 ); // block 1 is pixel data
          //std::cout << "vector has size : " << data.size() << std::endl;
          // Create a StandardPlane representing one sensor plane
          int id = 6+iPlane;
          StandardPlane plane(id, EVENT_TYPE, sensortype);
          plane.SetFlags(StandardPlane::FLAGS::FLAG_ACCUMULATE);

          // Size of one pixel data chunk: 12 bytes = 1+1+2+8 bytes for x,y,tot,ts
          int width = 256, height = 256;
          const unsigned int PIX_SIZE = 12;

       	  eutelescope::EUTelPixelDetector * currentDetector = 0x0;

          plane.SetSizeZS( width, height, ( data.size() ) / PIX_SIZE );

          std::vector<unsigned char> ZSDataX;
          std::vector<unsigned char> ZSDataY;
          std::vector<unsigned short> ZSDataTOT;
          std::vector<uint64_t> ZSDataTS;
          size_t offset = 0;
          unsigned char aWord = 0;
	  
	  for( unsigned int i = 0; i < ( data.size() ) / PIX_SIZE; i++ ) {
	    
	    ZSDataX   .push_back( unpackXorY( data, offset + sizeof( aWord ) * 0 ) );
	    ZSDataY   .push_back( unpackXorY( data, offset + sizeof( aWord ) * 1 ) );
	    ZSDataTOT .push_back( unpackTOT(  data, offset + sizeof( aWord ) * 2 ) );
	    ZSDataTS  .push_back( unpackTS(   data, offset + sizeof( aWord ) * 4 ) );
	    
	    offset += sizeof( aWord ) * PIX_SIZE;
	    
	    //std::cout << "[DATA] "  << " " << (int)ZSDataX[i] << " " << (int)ZSDataY[i] << " " << ZSDataTOT[i] << " " << ZSDataTS[i] << std::endl;
	  }
	  
	  // plane.SetSizeRaw(width, height);
	  // Set the trigger ID
	  plane.SetTLUEvent(GetTriggerID(source));
	  
	  // Add the plane to the StandardEvent
	  
	  //cout << "ZSDataX size" << ZSDataX.size() << endl;
	  
	  for(size_t i = 0 ; i<ZSDataX.size();i++){
	    
	    plane.SetPixel(i,ZSDataX[i],ZSDataY[i],ZSDataTOT[i]);

#ifdef WRITE_TOT	    
	    fprintf(totfile,"%i %i %i\n",ZSDataX[i],ZSDataY[i],ZSDataTOT[i]);
#endif	    
	  };
	  
	  //cout << "plane size" << plane.HitPixels() << endl;
	  
	  
	  /*---------------ZERO SUPP ---------------*/
	  
	  //printf("prepare a new TrackerData for the ZS data \n");
	  // prepare a new TrackerData for the ZS data
	  
	  mode = "ZS";
	  currentDetector = new eutelescope::EUTelTimepix3Detector;
	  zsFrame= new TrackerDataImpl;
	  currentDetector->setMode( mode );
	  zsDataEncoder["sensorID"] = plane.ID();
	  zsDataEncoder["sparsePixelType"] = eutelescope::kEUTelGenericSparsePixel;
	  zsDataEncoder.setCellID( zsFrame );
	  
	  size_t nPixel = plane.HitPixels();
	  //printf("EvSize=%d %d \n",EvSize,nPixel);
	  for (unsigned i = 0; i < nPixel; i++) {
	    zsFrame->chargeValues().push_back(plane.GetX(i));
	    zsFrame->chargeValues().push_back(255-plane.GetY(i));
	    //zsFrame->chargeValues().push_back(plane.GetPixel(i, 0));
	    zsFrame->chargeValues().push_back(ZSDataTOT[i]);
	    zsFrame->chargeValues().push_back(ZSDataTS[i]);


	    //if(plane.GetPixel(i, 0)>15) cout << "FFFFFFFFUUUUUUUUUUUUUUUUUUUUUUUUUUU" << endl;
	    
	  }
	  
	  zsDataCollection->push_back( zsFrame);
	  
	  if (  zsDataCollection->size() != 0 ) {
	    result.addCollection( zsDataCollection.release(), "zsdata_timepix3" );
	  }
      } //end of plane loop
      
      if ( result.getEventNumber() == 0 ) {
	
	// do this only in the first event
	
	LCCollectionVec * timepixSetupCollection = NULL;
	bool timepixSetupExists = false;
	try {
	  timepixSetupCollection = static_cast< LCCollectionVec* > ( result.getCollection( "timepix3Setup" ) ) ;
	  timepixSetupExists = true;
	} catch (...) {
	  timepixSetupCollection = new LCCollectionVec( lcio::LCIO::LCGENERICOBJECT );
	}
	
	for ( size_t iPlane = 0 ; iPlane < setupDescription.size() ; ++iPlane ) {
	  
	  timepixSetupCollection->push_back( setupDescription.at( iPlane ) );
	  
	}
	
	if (!timepixSetupExists) {
	  
	  result.addCollection( timepixSetupCollection, "timepix3Setup" );
	  
	}
      }
      return true; 
    }
#endif
    
  private:
    
    // The constructor can be private, only one static instance is created
    // The DataConverterPlugin constructor must be passed the event type
    // in order to register this converter for the corresponding conversions
    // Member variables should also be initialized to default values here.
    Timepix3ConverterPlugin()
      : DataConverterPlugin(EVENT_TYPE), m_XMLConfig("")
    {}
    
    // Information extracted in Initialize() can be stored here:
    std::string m_XMLConfig;
    
    // The single instance of this converter plugin
    static Timepix3ConverterPlugin m_instance;
  }; // class Timepix3ConverterPlugin
  
  // Instantiate the converter plugin instance
  Timepix3ConverterPlugin Timepix3ConverterPlugin::m_instance;
  
} // namespace eudaq



