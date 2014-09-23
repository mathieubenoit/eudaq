#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/Utils.hh"

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

using namespace std;

namespace eudaq {

  static const char* EVENT_TYPE = "CCPX";
  
  // Declare a new class that inherits from DataConverterPlugin
  class CLICpixConverterPlugin : public DataConverterPlugin {
    
  public:
    
    
    // This should return the trigger ID (as provided by the TLU)
    // if it was read out, otherwise it can either return (unsigned)-1,
    // or be left undefined as there is already a default version.
    virtual unsigned GetTriggerID(const Event & ev) const {
      // Make sure the event is of class RawDataEvent
      if (const RawDataEvent * rev = dynamic_cast<const RawDataEvent *> (&ev)) 
      {
         if ( rev->NumBlocks() > 0 && rev->GetBlock(0).size() >= 4 ) 
         {
            std::vector<unsigned char> data8 = rev->GetBlock( 0 ); // block 0 is trigger data 
            unsigned int * data32 = (unsigned int *) &data8[0];
            return data32[0];
         }
      }
      return (unsigned)-1;
    }
    
    virtual bool GetStandardSubEvent(StandardEvent & sev, const Event & ev) const 
    {
      std::string sensortype = "CLICpix";
            
      // Unpack data
      const RawDataEvent * rev = dynamic_cast<const RawDataEvent *> ( &ev );
      std::cout << "[Number of blocks] " << rev->NumBlocks() << std::endl;

      std::vector<unsigned char> data = rev->GetBlock( 1 ); // block 1 is pixel data
      std::cout << "Vector has size : " << data.size() << std::endl;

      // Create a StandardPlane representing one sensor plane
      int id = 0;
      StandardPlane plane(id, EVENT_TYPE, sensortype);
      
      const unsigned int PIX_DATA_SIZE = 4;
      unsigned int hits= data.size()  / PIX_DATA_SIZE;
      
      // Set the number of pixels
      int width = 64, height = 64;
      plane.SetSizeZS( width, height, hits );
      
      std::vector<unsigned char> ZSDataX;
      std::vector<unsigned char> ZSDataY;
      std::vector<unsigned char> ZSDataTOT;
      std::vector<unsigned char> ZSDataTOA;
      
      unsigned char aWord = 0;
      
      for( unsigned int i = 0; i < data.size() ; i+=PIX_DATA_SIZE ) 
      {
          ZSDataX   .push_back( data[i+0] );
          ZSDataY   .push_back( data[i+1] );
          ZSDataTOT .push_back( data[i+2] );
          ZSDataTOA .push_back( data[i+3] );
      }

      // Set the trigger ID
      plane.SetTLUEvent( GetTriggerID(ev) );
      
      for( size_t i = 0 ; i < ZSDataX.size(); ++i ) 
      {
         plane.PushPixel( ZSDataX[i], ZSDataY[i], ZSDataTOT[i] );
         std::cout << "[DATA] "  << " " << (int)ZSDataX[i] << " " << (int)ZSDataY[i] << " " << ZSDataTOT[i] << " " << ZSDataTOA[i] << std::endl;
      }

      // Add the plane to the StandardEvent
      sev.AddPlane( plane );
      
      // Indicate that data was successfully converted
      return true;
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

          std::string sensortype = "CLICpix";

          // Unpack data
          const RawDataEvent * rev = dynamic_cast<const RawDataEvent *> ( &source );
          //std::cout << "[Number of blocks] " << rev->NumBlocks() << std::endl;

          std::vector<unsigned char> data = rev->GetBlock( 1 ); // block 1 is pixel data
          //std::cout << "Vector has size : " << data.size() << std::endl;

          // Create a StandardPlane representing one sensor plane
          int id = 6;
          StandardPlane plane(id+iPlane, EVENT_TYPE, sensortype);

          const unsigned int PIX_DATA_SIZE = 4;
          unsigned int hits= data.size()  / PIX_DATA_SIZE;

          eutelescope::EUTelPixelDetector * currentDetector = 0x0;

          // Set the number of pixels
          int width = 64, height = 64;
          plane.SetSizeZS( width, height, hits );

          std::vector<unsigned char> ZSDataX;
          std::vector<unsigned char> ZSDataY;
          std::vector<unsigned short> ZSDataTOT;
          std::vector<uint64_t> ZSDataTOA;


          unsigned char aWord = 0;

          for( unsigned int i = 0; i < data.size() ; i+=PIX_DATA_SIZE )
          {
              ZSDataX   .push_back( data[i+0] );
              ZSDataY   .push_back( data[i+1] );
              ZSDataTOT .push_back( (unsigned short)data[i+2] );
              ZSDataTOA .push_back( (uint64_t)data[i+3] );
          }

          // Set the trigger ID
          plane.SetTLUEvent( GetTriggerID(source) );

          for( size_t i = 0 ; i < ZSDataX.size(); ++i )
          {
             plane.SetPixel( i,ZSDataX[i], ZSDataY[i], ZSDataTOT[i] );
             //std::cout << "[DATA] "  << " " << (int)ZSDataX[i] << " " << (int)ZSDataY[i] << " " << ZSDataTOT[i] << " " << ZSDataTOA[i] << std::endl;
          }



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
        	  zsFrame->chargeValues().push_back(plane.GetY(i));
        	  zsFrame->chargeValues().push_back(ZSDataTOT[i]);
        	  zsFrame->chargeValues().push_back(ZSDataTOA[i]);

          }

          zsDataCollection->push_back( zsFrame);

          if (  zsDataCollection->size() != 0 ) {
        	  result.addCollection( zsDataCollection.release(), "zsdata_clicpix" );
          }
      } //end of plane loop

    if ( result.getEventNumber() == 0 ) {

	// do this only in the first event

	LCCollectionVec * timepixSetupCollection = NULL;
	bool timepixSetupExists = false;
	try {
	  timepixSetupCollection = static_cast< LCCollectionVec* > ( result.getCollection( "clicpixSetup" ) ) ;
	  timepixSetupExists = true;
	} catch (...) {
	  timepixSetupCollection = new LCCollectionVec( lcio::LCIO::LCGENERICOBJECT );
	}

	for ( size_t iPlane = 0 ; iPlane < setupDescription.size() ; ++iPlane ) {

	  timepixSetupCollection->push_back( setupDescription.at( iPlane ) );

	}

	if (!timepixSetupExists) {

	  result.addCollection( timepixSetupCollection, "clicpixSetup" );

	}
      }
      return true;
    }
#endif


    
  private:
    
    CLICpixConverterPlugin()
      : DataConverterPlugin(EVENT_TYPE)
    {}
    
    static CLICpixConverterPlugin m_instance;
  }; // class CLICpixConverterPlugin
  
  // Instantiate the converter plugin instance
  CLICpixConverterPlugin CLICpixConverterPlugin::m_instance;
  
} // namespace eudaq
