#include "ClassFlowMakeImage.h"
#include "Helper.h"
#include "ClassLogFile.h"

#include "CImageBasis.h"
#include "ClassControllCamera.h"

#include <time.h>

// #define DEBUG_DETAIL_ON 

static const char* TAG = "flow_make_image";

esp_err_t ClassFlowMakeImage::camera_capture(){
    string nm =  namerawimage;
    Camera.CaptureToFile(nm);
    return ESP_OK;
}

void ClassFlowMakeImage::takePictureWithFlash(int flashdauer)
{
    Camera.CaptureToBasisImage(rawImage, flashdauer);
    if (SaveAllFiles) rawImage->SaveToFile(namerawimage);
}

void ClassFlowMakeImage::SetInitialParameter(void)
{
    waitbeforepicture = 5;
    isImageSize = false;
    ImageQuality = -1;    
    TimeImageTaken = 0;
    ImageQuality = 5;
    rawImage = NULL;
    ImageSize = FRAMESIZE_VGA;
    SaveAllFiles = false;
    namerawimage =  "/sdcard/img_tmp/raw.jpg";
}     


ClassFlowMakeImage::ClassFlowMakeImage(std::vector<ClassFlow*>* lfc) : ClassFlowImage(lfc, TAG)
{
    SetInitialParameter();
}

bool ClassFlowMakeImage::ReadParameter(FILE* pfile, string& aktparamgraph)
{
    std::vector<string> zerlegt;

    aktparamgraph = trim(aktparamgraph);

    if (aktparamgraph.size() == 0)
        if (!this->GetNextParagraph(pfile, aktparamgraph))
            return false;

    if (aktparamgraph.compare("[MakeImage]") != 0)       // Paragraph passt nich zu MakeImage
        return false;

    while (this->getNextLine(pfile, &aktparamgraph) && !this->isNewParagraph(aktparamgraph))
    {
        zerlegt = this->ZerlegeZeile(aktparamgraph);
        if ((zerlegt[0] ==  "LogImageLocation") && (zerlegt.size() > 1))
        {
            LogImageLocation = "/sdcard" + zerlegt[1];
            isLogImage = true;
        }
        if ((zerlegt[0] == "ImageQuality") && (zerlegt.size() > 1))
            ImageQuality = std::stod(zerlegt[1]);

        if ((zerlegt[0] == "ImageSize") && (zerlegt.size() > 1))
        {
            ImageSize = Camera.TextToFramesize(zerlegt[1].c_str());
            isImageSize = true;
        }

        if ((toUpper(zerlegt[0]) == "SAVEALLFILES") && (zerlegt.size() > 1))
        {
            if (toUpper(zerlegt[1]) == "TRUE")
                SaveAllFiles = true;
        }

    }

    Camera.SetQualitySize(ImageQuality, ImageSize);
    image_width = Camera.image_width;
    image_height = Camera.image_height;
    rawImage = new CImageBasis();
    rawImage->CreateEmptyImage(image_width, image_height, 3);

    return true;
}

string ClassFlowMakeImage::getHTMLSingleStep(string host)
{
    string result;
    result = "Raw Image: <br>\n<img src=\"" + host + "/img_tmp/raw.jpg\">\n";
    return result;
}

bool ClassFlowMakeImage::doFlow(string zwtime)
{
    string logPath = CreateLogFolder(zwtime);

    int flashdauer = (int) waitbeforepicture * 1000;
 
 #ifdef DEBUG_DETAIL_ON  
    LogFile.WriteHeapInfo("ClassFlowMakeImage::doFlow - Before takePictureWithFlash");
#endif

    takePictureWithFlash(flashdauer);

#ifdef DEBUG_DETAIL_ON  
    LogFile.WriteHeapInfo("ClassFlowMakeImage::doFlow - After takePictureWithFlash");
#endif

    LogImage(logPath, "raw", NULL, NULL, zwtime, rawImage);

    RemoveOldLogs();

#ifdef DEBUG_DETAIL_ON  
    LogFile.WriteHeapInfo("ClassFlowMakeImage::doFlow - After RemoveOldLogs");
#endif

    return true;
}

esp_err_t ClassFlowMakeImage::SendRawJPG(httpd_req_t *req)
{
    int flashdauer = (int) waitbeforepicture * 1000;
    return Camera.CaptureToHTTP(req, flashdauer);
}


ImageData* ClassFlowMakeImage::SendRawImage()
{
    CImageBasis *zw = new CImageBasis(rawImage);
    ImageData *id;
    int flashdauer = (int) waitbeforepicture * 1000;
    Camera.CaptureToBasisImage(zw, flashdauer);
    id = zw->writeToMemoryAsJPG();    
    delete zw;
    return id;  
}

time_t ClassFlowMakeImage::getTimeImageTaken()
{
    return TimeImageTaken;
}

ClassFlowMakeImage::~ClassFlowMakeImage(void)
{
    delete rawImage;
}

