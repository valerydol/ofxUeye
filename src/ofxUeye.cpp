#include "ofxUeye.h"
#include <uEye.h>

using namespace ofxMachineVision;

namespace ofxUeye {
	//----------
	Device::Device() {
		this->cameraHandle = NULL;
		this->imageMemoryID = 0;
	}

	//----------
	Specification Device::open(int deviceID) {
		HIDS cameraHandle = deviceID + 1001 | IS_USE_DEVICE_ID;
		int result;

		result = is_InitCamera(&cameraHandle, NULL);

		//upload firmware if we have to
		if (result == IS_STARTER_FW_UPLOAD_NEEDED) {
			int timeNeeded;
			is_GetDuration(cameraHandle, IS_SE_STARTER_FW_UPLOAD, &timeNeeded);
			OFXMV_WARNING << "Camera firmware upload required, please wait " << timeNeeded << "s";
			cameraHandle = (HIDS) (((INT) cameraHandle) | IS_ALLOW_STARTER_FW_UPLOAD);
			result = is_InitCamera(&cameraHandle);
		}

		if (result != IS_SUCCESS) {
			OFXMV_ERROR << "Couldn't initialise camera";
			return Specification();
		}

		this->cameraHandle = cameraHandle;
		
		BOARDINFO cameraInfo;
		is_GetCameraInfo(this->cameraHandle, &cameraInfo);
		SENSORINFO sensorInfo;
		is_GetSensorInfo(this->cameraHandle, &sensorInfo);

		auto specification = Specification(sensorInfo.nMaxWidth, sensorInfo.nMaxHeight, "IDS Imaging", cameraInfo.ID, cameraInfo.SerNo);

		this->pixels.allocate(specification.getSensorWidth(), specification.getSensorHeight(), OF_IMAGE_GRAYSCALE);
		is_SetAllocatedImageMem(this->cameraHandle, specification.getSensorWidth(), specification.getSensorHeight(), 8, (char *) this->pixels.getPixels(), &this->imageMemoryID);
		is_SetImageMem(this->cameraHandle, (char *) this->pixels.getPixels(), this->imageMemoryID);

		//setup some camera parameters
		is_SetColorMode(this->cameraHandle, IS_CM_SENSOR_RAW8);
		result = is_SetOptimalCameraTiming(this->cameraHandle, IS_BEST_PCLK_RUN_ONCE, 4000, &this->maxClock, &this->fps);;

		specification.addFeature(Feature::Feature_Binning);
		specification.addFeature(Feature::Feature_DeviceID);
		specification.addFeature(Feature::Feature_Exposure);
		specification.addFeature(Feature::Feature_FreeRun);
		specification.addFeature(Feature::Feature_Gain);
		specification.addFeature(Feature::Feature_GPO);
		specification.addFeature(Feature::Feature_OneShot);
		specification.addFeature(Feature::Feature_PixelClock);
		specification.addFeature(Feature::Feature_ROI);
		specification.addFeature(Feature::Feature_Triggering);

		return specification;
	}

	//----------
	void Device::close() {
		is_ExitCamera(this->cameraHandle);
		this->cameraHandle = NULL;
	}

	//----------
	bool Device::startCapture() {
		if (is_CaptureVideo(this->cameraHandle, IS_DONT_WAIT) != IS_SUCCESS) {
			OFXMV_ERROR << "Couldn't start capture";
			return false;
		} else {
			return true;
		}
	}

	//----------
	void Device::stopCapture() {
		if (!is_FreezeVideo(this->cameraHandle, IS_DONT_WAIT)) {
			OFXMV_ERROR << "Couldn't stop capture";
		}
	}

	//----------
	void Device::setExposure(Microseconds exposureMicros) {
		double autoExposure = 0.0;
		if (is_SetAutoParameter(this->cameraHandle, IS_SET_ENABLE_AUTO_SHUTTER, &autoExposure, 0) != IS_SUCCESS) {
			OFXMV_ERROR << "Couldn't stop auto exposure";
		}

		double exposureMillis = (double) exposureMicros / 1000.0;
		if (is_Exposure(this->cameraHandle, IS_EXPOSURE_CMD_SET_EXPOSURE, &exposureMillis, 1) != IS_SUCCESS) {
			OFXMV_ERROR << "Couldn't set exposure";
		}
	}

	//----------
	void Device::setGain(float gain) {
		double autoGain = 0.0;
		if (is_SetAutoParameter(this->cameraHandle, IS_SET_ENABLE_AUTO_GAIN, &autoGain, 0) != IS_SUCCESS) {
			OFXMV_ERROR << "Couldn't stop auto gain";
		}

		double gainDouble = (double) gain * 100.0;
		if (is_SetHWGainFactor(this->cameraHandle, IS_SET_MASTER_GAIN_FACTOR, gainDouble) != IS_SUCCESS) {
			OFXMV_ERROR << "Couldn't set gain";
		}
	}

	//----------
	void Device::setBinning(int binningX, int binningY) {
		int result = IS_SUCCESS;
		if (binningX < 1 || binningY < 1 || binningX > 16 || binningY > 16) {
			OFXMV_ERROR << "Can't set a binning value of less than 1 or more than 16";
		} else if (binningX == 1 && binningY == 1) {
			result = is_SetBinning(this->cameraHandle, IS_BINNING_DISABLE);
		} else {
			int horizontalFlag = IS_BINNING_DISABLE;
			switch (binningX) {
			case 2:
				horizontalFlag = IS_BINNING_2X_HORIZONTAL;
				break;
			case 3:
				horizontalFlag = IS_BINNING_3X_HORIZONTAL;
				break;
			case 4:
				horizontalFlag = IS_BINNING_4X_HORIZONTAL;
				break;
			case 5:
				horizontalFlag = IS_BINNING_5X_HORIZONTAL;
				break;
			case 6:
				horizontalFlag = IS_BINNING_6X_HORIZONTAL;
				break;
			case 8:
				horizontalFlag = IS_BINNING_8X_HORIZONTAL;
				break;
			case 16:
				horizontalFlag = IS_BINNING_16X_HORIZONTAL;
				break;
			default:
				OFXMV_ERROR << "Cannot set binning, please check manual for valid values";
				break;
			}
			if (horizontalFlag != IS_BINNING_DISABLE) {
				if (is_SetBinning(this->cameraHandle, horizontalFlag) != IS_SUCCESS) {
					OFXMV_ERROR << "Failed to set horizontal binning";
				}
			}

			int verticalFlag = IS_BINNING_DISABLE;
			switch (binningX) {
			case 2:
				verticalFlag = IS_BINNING_2X_VERTICAL;
				break;
			case 3:
				verticalFlag = IS_BINNING_3X_VERTICAL;
				break;
			case 4:
				verticalFlag = IS_BINNING_4X_VERTICAL;
				break;
			case 5:	
				verticalFlag = IS_BINNING_5X_VERTICAL;
				break;
			case 6:	
				verticalFlag = IS_BINNING_6X_VERTICAL;
				break;
			case 8:	
				verticalFlag = IS_BINNING_8X_VERTICAL;
				break;
			case 16:
				verticalFlag = IS_BINNING_16X_VERTICAL;
				break;
			default:
				OFXMV_ERROR << "Cannot set binning, please check manual for valid values";
				break;
			}
			if (verticalFlag != IS_BINNING_DISABLE) {
				if (is_SetBinning(this->cameraHandle, verticalFlag) != IS_SUCCESS) {
					OFXMV_ERROR << "Failed to set vertical binning";
				}
			}
		}
	}

	//----------
	void Device::setROI(const ofRectangle & roi) {
		IS_RECT aoiRect;
		aoiRect.s32X = (int) roi.x | IS_AOI_IMAGE_POS_ABSOLUTE;
		aoiRect.s32Y = (int) roi.y | IS_AOI_IMAGE_POS_ABSOLUTE;
		aoiRect.s32Width = (int) roi.width;
		aoiRect.s32Height = (int) roi.height;

		is_AOI(this->cameraHandle, IS_AOI_IMAGE_SET_AOI, &aoiRect, sizeof(aoiRect));
	}
}