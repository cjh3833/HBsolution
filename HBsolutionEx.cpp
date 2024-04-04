// pylon API�� ����ϱ� ���� ��� ���� ����.
#include <pylon/PylonIncludes.h>
// openCV�� ����ϱ� ���� �������
#include <opencv2/opencv.hpp> 
//time�� ���� ���� �������
#include <time.h> 

#ifdef PYLON_WIN_BUILD
#    include <pylon/PylonGUI.h> // Windows ȯ�濡�� GUI ���� ����� ����ϱ� ���� ��� ����.
#endif


// pylon ��ü ����� ���� ���ӽ����̽�.
// cout ����� ���� ���ӽ����̽�.
using namespace Pylon;
// cv ��ü ����� ���� ���ӽ����̽�
// std ����� ���� ���ӽ����̽�
using namespace cv;
using namespace std;

// ĸó�� �̹����� ���� ����.
static const uint32_t c_countOfImagesToGrab = 300;

int main(int /*argc*/, char* /*argv*/[])
{
    // �̹��� ��� ������ ������ ����
    double minVal, maxVal;
    Point minLoc, maxLoc;
    Point matchLoc;
    Mat result;
    int method = 1;

    //�Ӱ谪 ����
    double threshold = 0.02;

    //�ð� ����� ������ ��
    int time_watch = 0;

    // ���� ���ø����̼��� ���� �ڵ�.
    int exitCode = 0;

    // pylon �޼ҵ带 ����ϱ� ���� pylon ��Ÿ���� �ʱ�ȭ�ؾ� ��.
    PylonInitialize();

    //�ۺ� �̹��� ����
    Mat templ = imread("templ_4_1.png");
    Mat templ2 = imread("templ_5.png");

    try
    {
        // ù ��°�� ã�� ī�޶� ��ġ�� �ν���Ʈ ī�޶� ��ü ����.
        CInstantCamera camera(CTlFactory::GetInstance().CreateFirstDevice());

        // ī�޶��� �� �̸� ���.
        cout << "Using device " << camera.GetDeviceInfo().GetModelName() << endl;

        // MaxNumBuffer �Ķ���ʹ� ĸó�� ���� �Ҵ�� ������ ���� �����ϴ� �� ���� �� ����.
        // �� �Ķ������ �⺻���� 10.
        camera.MaxNumBuffer = 5;

        // c_countOfImagesToGrab �̹����� ĸó ����.
        // ī�޶� ��ġ�� ���� ����� �����ϴ� �⺻ �������� �Ķ����ȭ��.
        camera.StartGrabbing(c_countOfImagesToGrab);

        // �� ����Ʈ �����ʹ� ĸó ��� �����͸� �ް� ��.
        CGrabResultPtr ptrGrabResult;  //ĸ�İ���� ������ �̰� ���ϸ�Ī�� �����Ű�� ?
        Mat test, src;
        Mat test2, src2;
        
        
        
        
        // �� ?
        CImageFormatConverter formatConverter;


        formatConverter.OutputPixelFormat = PixelType_BGR8packed;
        CPylonImage pylonImage;


        // c_countOfImagesToGrab �̹����� �˻��Ǿ��� �� Camera.StopGrabbing()�� �ڵ����� ȣ���.
        while (camera.IsGrabbing())
        {
            //�ð������ �����ֱ� ���� ����
            time_watch++;

            // �̹����� ��ٸ� ���� �˻�. 5000ms�� Ÿ�Ӿƿ� ���.
            camera.RetrieveResult(5000, ptrGrabResult, TimeoutHandling_ThrowException);

            

            // �̹����� ���������� ĸó�Ǿ��°�?
            if (ptrGrabResult->GrabSucceeded())
            {
                //�̹��� �޾ƿ��� src����ȯ �� imshow
                //Mat src(ptrGrabResult->GetHeight(), ptrGrabResult->GetWidth(), CV_8UC1, ptrGrabResult->GetBuffer());

                // �̹��� �����Ϳ� ����.  // ����~ĸ�ĵ� �̹����� ǥ�� �ϴ� �κп��� grab ��������
                cout << "SizeX: " << ptrGrabResult->GetWidth() << endl;
                cout << "SizeY: " << ptrGrabResult->GetHeight() << endl;
                const uint8_t* pImageBuffer = (uint8_t*)ptrGrabResult->GetBuffer();
                cout << "Gray value of first pixel: " << (uint32_t)pImageBuffer[0] << endl;
                cout << "time : " << time_watch << endl;
                

                //�̹��� �޾ƿ��� src����ȯ �� imshow
                //gray�� ��ȯ���־��⶧���� CV_8UC3���� ���� dvd
                //src = cv::Mat(ptrGrabResult->GetHeight(), ptrGrabResult->GetWidth(), CV_8UC3, (uint8_t*)ptrGrabResult->GetBuffer());

                // �� ?
                formatConverter.Convert(pylonImage, ptrGrabResult);
                Mat src = cv::Mat(ptrGrabResult->GetHeight(), ptrGrabResult->GetWidth(), CV_8UC3, (uint8_t*)pylonImage.GetBuffer());


                Mat img_out;
                src.copyTo(img_out);

                /* ��Ī ���
                0: TM_SQDIFF (��ġ�ϸ� �Ҽ��� ���� �۾���)
                1: TM_SQDIFF NORMED (��ġ�ϸ� �Ҽ��� ���� �۾���)
                2: TM CCORR
                3: TM CCORR NORMED
                4: TM COEFF
                5: TM COEFF NORMED";
                */


                // ���� �̹������� ���a�� �̹����� ��ġ�ϴ� ������ ã�� �˰���
                matchTemplate(src, templ, result, method);
                // normalize�� �̿��ؼ� �̹��� ����ȭ, ������ ����, 0~1���� ����
                normalize(result, result, 0, 1, NORM_MINMAX, -1, Mat());

                // �־��� ����� �ּҰ�, �ִ밪�� ã�� �Լ��� �ּҰ�, �ִ밪�� �ִ� ��ǥ������ �Բ� �˾Ƴ� �� ����
                //Val -> �� ǥ��,     Loc -> ��ǥǥ��
                minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc, Mat());

                for (int i = 0; i < result.rows; i++) {
                    for (int j = 0; j < result.cols; j++) {
                        if (result.at<float>(i, j) <= threshold) {
                            // �Ӱ谪 �̻��� ��ǥ�� template(pattern) ũ�⸸ŭ�� ���� �簢���� �׸�.
                            // OpenCV�� ��� RGB ������ �ƴ� BGR ������ ǥ����.
                            rectangle(img_out, Point(j, i), Point(j + templ.cols, i + templ.rows), Scalar(0, 0, 255), 1);
                        }
                    }
                }


                //cvtColor �Լ��� �̿��Ͽ� ��������� gray�� ����  //�ڵ� �����ϰ� �� �� ������ ����
                //cvtColor(result, result, COLOR_GRAY2BGR);

                /*
                matchLoc = maxLoc;
                rectangle(img_out, matchLoc, Point(matchLoc.x + templ.cols, matchLoc.y + templ.rows), Scalar(0, 0, 255), 1);
                */

                /*
                matchLoc = minLoc;

                if (maxVal >= threshold) 
                {
                //matchLoc�� �׸�� ��ǥ �����
                rectangle(img_out, matchLoc, Point(matchLoc.x + templ.cols, matchLoc.y + templ.rows), Scalar(0, 0, 255), 1);

                //GRAY�� ã�� ���� ���������� ���׶�� �����   
                circle(result, matchLoc, 3, Scalar(0, 0, 255), 1);
                //rectangle(result, matchLoc, Point(matchLoc.x + templ.cols, matchLoc.y + templ.rows), Scalar(0, 0, 255), 1);

                cout << "minVal : " << minVal  << endl;
                cout << "maxVal : " << maxVal << endl << endl;
                }
                */
                

                 // imshow �̹��� ��� �Լ�
                 imshow("src", img_out);
                 imshow("templ", templ);
                 imshow("result", result);
                    
                 waitKey(1);
                

#ifdef PYLON_WIN_BUILD
                // ĸó�� �̹����� ǥ��.
                //Pylon::DisplayImage(1, ptrGrabResult);
#endif
            }
            else
            {
                // ���� �޽��� ���.
                cout << "Error �ݺ��: " << std::hex << ptrGrabResult->GetErrorCode() << std::dec << " " << ptrGrabResult->GetErrorDescription() << endl;
            }
        }
    }
    catch (const GenericException& e)
    {
        // ���� ó��.
        cerr << "An exception occurred." << endl
            << e.GetDescription() << endl;
        exitCode = 1;
    }

    // ���� ���� ��⸦ ��Ȱ��ȭ�Ϸ��� ���� �� ���� �ּ� ó��.
    cerr << endl << "Press enter to exit." << endl;
    while (cin.get() != '\n');

    // ��� pylon ���ҽ��� ����.
    PylonTerminate();

    return exitCode;
}