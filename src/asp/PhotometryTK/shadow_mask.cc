#include <boost/foreach.hpp>
#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include <vw/FileIO.h>
#include <vw/Image.h>
#include <vw/Cartography.h>
using namespace vw;

struct Options {
  Options() : nodata(-1), threshold(-1) {}
  // Input
  std::vector<std::string> input_files;
  double nodata, threshold;
};

// Operational Code
template <class PixelT>
void shadow_mask_nodata( Options& opt,
                         std::string input,
                         std::string output ) {
  VW_ASSERT( opt.nodata != -1, ArgumentErr() << "Missing user supplied nodata value" );
  typedef typename PixelChannelType<PixelT>::type ChannelT;
  typedef typename MaskedPixelType<PixelT>::type PMaskT;
  if ( opt.threshold < 0 ) {
    ChannelRange<ChannelT> helper;
    opt.threshold = 0.1*helper.max();
  }

  cartography::GeoReference georef;
  cartography::read_georeference(georef, input);
  DiskImageView<PixelT> input_image(input);
  ImageViewRef<PMaskT> masked_input = create_mask(input_image,opt.nodata);
  ImageViewRef<PMaskT> result =
    intersect_mask(masked_input,create_mask(threshold(input_image,opt.threshold)));
  cartography::write_georeferenced_image(output, create_mask(result,opt.nodata),
                                         georef, TerminalProgressCallback("photometrytk","Writing:"));
}

template <class PixelT>
void shadow_mask_alpha( Options& opt,
                        std::string input,
                        std::string output ) {
  typedef typename PixelChannelType<PixelT>::type ChannelT;
  typedef typename PixelWithoutAlpha<PixelT>::type PNAlphaT;
  typedef typename MaskedPixelType<PNAlphaT>::type PMaskT;
  if ( opt.threshold < 0 ) {
    ChannelRange<ChannelT> helper;
    opt.threshold = 0.1*helper.max();
  }

  cartography::GeoReference georef;
  cartography::read_georeference(georef, input);
  DiskImageView<PixelT> input_image(input);
  ImageViewRef<PMaskT> masked_input = alpha_to_mask(input_image);
  ImageViewRef<PMaskT> result =
    intersect_mask(masked_input,create_mask(threshold(apply_mask(masked_input),opt.threshold)));
  cartography::write_georeferenced_image(output, mask_to_alpha(result), georef,
                                         TerminalProgressCallback("photometrytk","Writing:"));
}

// Code for handing input
void handle_arguments( int argc, char *argv[], Options& opt ) {
  po::options_description general_options("");
  general_options.add_options()
    ("nodata-value", po::value<double>(&opt.nodata), "Value that is nodata in the input image. Not used if input has alpha.")
    ("threshold,t", po::value<double>(&opt.threshold), "Value used to detect shadows.")
    ("help,h", "Display this help message");

  po::options_description positional("");
  positional.add_options()
    ("input-files", po::value<std::vector<std::string> >(&opt.input_files));

  po::positional_options_description positional_desc;
  positional_desc.add("input-files", -1);

  po::options_description all_options;
  all_options.add(general_options).add(positional);

  po::variables_map vm;
  try {
    po::store( po::command_line_parser( argc, argv ).options(all_options).positional(positional_desc).run(), vm );
    po::notify( vm );
  } catch (po::error &e) {
    vw_throw( ArgumentErr() << "Error parsing input:\n\t"
              << e.what() << general_options );
  }

  std::ostringstream usage;
  usage << "Usage: " << argv[0] << " [options] <image-files>\n";

  if ( vm.count("help") )
    vw_throw( ArgumentErr() << usage.str() << general_options );
  if ( opt.input_files.empty() )
    vw_throw( ArgumentErr() << "Missing input files!\n"
              << usage.str() << general_options );
}

int main( int argc, char *argv[] ) {

  Options opt;
  try {
    handle_arguments( argc, argv, opt );

    BOOST_FOREACH( const std::string& input, opt.input_files ) {

      // Determining the format of the input
      DiskImageResource *rsrc = DiskImageResource::open(input);
      ChannelTypeEnum channel_type = rsrc->channel_type();
      PixelFormatEnum pixel_format = rsrc->pixel_format();
      delete rsrc;

      vw_out() << "Loading: " << input << "\n";
      size_t pt_idx = input.rfind(".");
      std::string output = input.substr(0,pt_idx)+"_shdw" +
        input.substr(pt_idx,input.size()-pt_idx);

      switch (pixel_format) {
      case VW_PIXEL_GRAY:
        switch (channel_type) {
        case VW_CHANNEL_UINT8:
          shadow_mask_nodata<PixelGray<uint8> >( opt, input, output ); break;
        case VW_CHANNEL_INT16:
          shadow_mask_nodata<PixelGray<int16> >( opt, input, output ); break;
        case VW_CHANNEL_UINT16:
          shadow_mask_nodata<PixelGray<uint16> >( opt, input, output ); break;
        default:
          shadow_mask_nodata<PixelGray<float32> >( opt, input, output ); break;
        }
        break;
      case VW_PIXEL_GRAYA:
        switch (channel_type) {
        case VW_CHANNEL_UINT8:
          shadow_mask_alpha<PixelGrayA<uint8> >( opt, input, output ); break;
        case VW_CHANNEL_INT16:
          shadow_mask_alpha<PixelGrayA<int16> >( opt, input, output ); break;
        case VW_CHANNEL_UINT16:
          shadow_mask_alpha<PixelGrayA<uint16> >( opt, input, output ); break;
        default:
          shadow_mask_alpha<PixelGrayA<float32> >( opt, input, output ); break;
        }
        break;
      case VW_PIXEL_RGB:
        switch (channel_type) {
        case VW_CHANNEL_UINT8:
          shadow_mask_nodata<PixelRGB<uint8> >( opt, input, output ); break;
        case VW_CHANNEL_INT16:
          shadow_mask_nodata<PixelRGB<int16> >( opt, input, output ); break;
        case VW_CHANNEL_UINT16:
          shadow_mask_nodata<PixelRGB<uint16> >( opt, input, output ); break;
        default:
          shadow_mask_nodata<PixelRGB<float32> >( opt, input, output ); break;
        }
        break;
      default:
        switch (channel_type) {
        case VW_CHANNEL_UINT8:
          shadow_mask_alpha<PixelRGBA<uint8> >( opt, input, output ); break;
        case VW_CHANNEL_INT16:
          shadow_mask_alpha<PixelRGBA<int16> >( opt, input, output ); break;
        case VW_CHANNEL_UINT16:
          shadow_mask_alpha<PixelRGBA<uint16> >( opt, input, output ); break;
        default:
          shadow_mask_alpha<PixelRGBA<float32> >( opt, input, output ); break;
        }
        break;
      }

    }

  } catch ( const ArgumentErr& e ) {
    vw_out() << e.what() << std::endl;
    return 1;
  } catch ( const Exception& e ) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}