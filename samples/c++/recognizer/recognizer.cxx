/* Copyright (C) 2011-2020 Doubango Telecom <https://www.doubango.org>
* File author: Mamadou DIOP (Doubango Telecom, France).
* License: For non commercial use only.
* Source code: https://github.com/DoubangoTelecom/ultimateMICR-SDK
* WebSite: https://www.doubango.org/webapps/micr/
*/

/*
	https://github.com/DoubangoTelecom/ultimateMICR/blob/master/SDK_dist/samples/c++/recognizer/README.md
	Usage: 
		recognizer \
			--image <path-to-image-with-micr-zone-to-recognize> \
			[--assets <path-to-assets-folder>] \
			[--tokenfile <path-to-license-token-file>] \
			[--tokendata <base64-license-token-data>]

	Example:
		recognizer \
			--image C:/Projects/GitHub/ultimate/ultimateMICR/SDK_dist/assets/images/e13b_1280x720.jpg \
			--assets C:/Projects/GitHub/ultimate/ultimateMICR/SDK_dist/assets \
			--tokenfile C:/Projects/GitHub/ultimate/ultimateMICR/SDK_dev/tokens/windows-iMac.lic
		
*/

#include <ultimateMICR-SDK-API-PUBLIC.h>
#include "../micr_utils.h"
#if defined(_WIN32)
#include <algorithm> // std::replace
#endif

using namespace ultimateMicrSdk;

// Configuration for ANPR deep learning engine
static const char* __jsonConfig =
"{"
"\"debug_level\": \"info\","
"\"debug_write_input_image_enabled\": false,"
"\"debug_internal_data_path\": \".\","
""
"\"num_threads\": -1,"
"\"gpgpu_enabled\": true,"
""
"\"segmenter_accuracy\": \"high\","
"\"interpolation\": \"bilinear\","
"\"roi\": [0, 0, 0, 0],"
"\"min_score\": 0.3,"
"\"score_type\": \"min\""
"";

// Asset manager used on Android to files in "assets" folder
#if ULTMICR_SDK_OS_ANDROID 
#	define ASSET_MGR_PARAM() __sdk_android_assetmgr, 
#else
#	define ASSET_MGR_PARAM() 
#endif /* ULTMICR_SDK_OS_ANDROID */

static void printUsage(const std::string& message = "");

/*
* Entry point
*/
int main(int argc, char *argv[])
{
	// local variables
	UltMicrSdkResult result(0, "OK", "{}");
	std::string assetsFolder, licenseTokenData, licenseTokenFile;
	std::string pathFileImage;

	// Parsing args
	std::map<std::string, std::string > args;
	if (!micrParseArgs(argc, argv, args)) {
		printUsage();
		return -1;
	}
	if (args.find("--image") == args.end()) {
		printUsage("--image required");
		return -1;
	}
	pathFileImage = args["--image"];
		
	if (args.find("--assets") != args.end()) {
		assetsFolder = args["--assets"];
#if defined(_WIN32)
		std::replace(assetsFolder.begin(), assetsFolder.end(), '\\', '/');
#endif
	}
	if (args.find("--tokenfile") != args.end()) {
		licenseTokenFile = args["--tokenfile"];
#if defined(_WIN32)
		std::replace(licenseTokenFile.begin(), licenseTokenFile.end(), '\\', '/');
#endif
	}
	if (args.find("--tokendata") != args.end()) {
		licenseTokenData = args["--tokendata"];
	}

	// Update JSON config
	std::string jsonConfig = __jsonConfig;
	if (!assetsFolder.empty()) {
		jsonConfig += std::string(",\"assets_folder\": \"") + assetsFolder + std::string("\"");
	}
	if (!licenseTokenFile.empty()) {
		jsonConfig += std::string(",\"license_token_file\": \"") + licenseTokenFile + std::string("\"");
	}
	if (!licenseTokenData.empty()) {
		jsonConfig += std::string(",\"license_token_data\": \"") + licenseTokenData + std::string("\"");
	}
	
	jsonConfig += "}"; // end-of-config

	// Decode image
	MicrFile fileImage;
	if (!micrDecodeFile(pathFileImage, fileImage)) {
		ULTMICR_SDK_PRINT_INFO("Failed to read image file: %s", pathFileImage.c_str());
		return -1;
	}

	// Init
	ULTMICR_SDK_PRINT_INFO("Starting recognizer...");
	ULTMICR_SDK_ASSERT((result = UltMicrSdkEngine::init(
		ASSET_MGR_PARAM()
		jsonConfig.c_str()
	)).isOK());

	// Recognize/Process
	ULTMICR_SDK_ASSERT((result = UltMicrSdkEngine::process(
		fileImage.type, // If you're using data from your camera then, the type would be YUV-family instead of RGB-family. https://www.doubango.org/SDKs/ccard/docs/cpp-api.html#_CPPv4N15ultimateMICRSdk22ULTMICR_SDK_IMAGE_TYPEE
		fileImage.uncompressedData,
		fileImage.width,
		fileImage.height
	)).isOK());
	ULTMICR_SDK_PRINT_INFO("Processing done.");

	// Print latest result
	const std::string& json_ = result.json();
	if (!json_.empty()) {
		ULTMICR_SDK_PRINT_INFO("result: %s", json_.c_str());
	}

	ULTMICR_SDK_PRINT_INFO("Press any key to terminate !!");
	getchar();

	// DeInit
	ULTMICR_SDK_PRINT_INFO("Ending recognizer...");
	ULTMICR_SDK_ASSERT((result = UltMicrSdkEngine::deInit()).isOK());

	return 0;
}

/*
* Print usage
*/
static void printUsage(const std::string& message /*= ""*/)
{
	if (!message.empty()) {
		ULTMICR_SDK_PRINT_ERROR("%s", message.c_str());
	}

	ULTMICR_SDK_PRINT_INFO(
		"\n********************************************************************************\n"
		"recognizer\n"
		"\t--image <path-to-image-with-micr-zone-to-recognize> \n"
		"\t[--assets <path-to-assets-folder>] \n"
		"\t[--tokenfile <path-to-license-token-file>] \n"
		"\t[--tokendata <base64-license-token-data>] \n"
		"\n"
		"Options surrounded with [] are optional.\n"
		"\n"
		"--image: Path to the image(JPEG/PNG/BMP) to process. You can use default image at ../../../assets/images/e13b_1280x720.jpg.\n\n"
		"--assets: Path to the assets folder containing the configuration files and models. Default value is the current folder.\n\n"
		"--tokenfile: Path to the file containing the base64 license token if you have one. If not provided then, the application will act like a trial version. Default: null.\n\n"
		"--tokendata: Base64 license token if you have one. If not provided then, the application will act like a trial version. Default: null.\n\n"
		"********************************************************************************\n"
	);
}
