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
using namespace std;


// cv ��ü ����� ���� ���ӽ����̽�
// std ����� ���� ���ӽ����̽�
using namespace cv;
using namespace std;

// ĸó�� �̹����� ���� ����.
static const uint32_t c_countOfImagesToGrab = 100;



int main(int /*argc*/, char* /*argv*/[])
{


    // �̹��� ��� ������ ������ ����
    clock_t start, end;
    double minVal, maxVal;
    Point minLoc, maxLoc;
    Point matchLoc;
    Mat result;
    //�ð� ����� ������ ��
    int time_watch = 0;

    // ���� ���ø����̼��� ���� �ڵ�.
    int exitCode = 0;

    // pylon �޼ҵ带 ����ϱ� ���� pylon ��Ÿ���� �ʱ�ȭ�ؾ� ��.
    PylonInitialize();

    //�ۺ� �̹��� ����
    Mat templ = imread("templ.png");


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
                Mat src(ptrGrabResult->GetHeight(), ptrGrabResult->GetWidth(), CV_8UC1, ptrGrabResult->GetBuffer());


                

                // �̹��� �����Ϳ� ����.  // ����~ĸ�ĵ� �̹����� ǥ�� �ϴ� �κп��� grab ��������
                cout << "SizeX: " << ptrGrabResult->GetWidth() << endl;
                cout << "SizeY: " << ptrGrabResult->GetHeight() << endl;
                const uint8_t* pImageBuffer = (uint8_t*)ptrGrabResult->GetBuffer();
                cout << "Gray value of first pixel: " << (uint32_t)pImageBuffer[0] << endl;
                cout << "time : " << time_watch << endl << endl;

                for (int i = 0; i < 6; i++)
                {
                    Mat img_out;
                    src.copyTo(img_out);

                    int Matching_method = i;
                    /*
                    0: TM_SQDIFF
                    1: TM_SQDIFF NORMED
                    2: TM CCORR
                    3: TM CCORR NORMED
                    4: TM COEFF
                    5: TM COEFF NORMED";
                    */

                    start = clock();

                    // ���� �̹������� ���a�� �̹����� ��ġ�ϴ� ������ ã�� �˰���
                    matchTemplate(src, templ, result, i); //���⼭ ����

                    // normalize�� �̿��ؼ� �̹��� ����ȭ, ������ ����, 0~1���� ����
                    normalize(result, result, 0, 1, NORM_MINMAX, -1, Mat());

                    // �־��� ����� �ּҰ�, �ִ밪�� ã�� �Լ��� �ּҰ�, �ִ밪�� �ִ� ��ǥ������ �Բ� �˾Ƴ� �� ����
                    minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc, Mat());

                    //1,2��° �ݺ��������� minLoc
                    // 3~6��° �ݺ��������� maxLoc
                    if (Matching_method == 0 || Matching_method == 1)
                    {
                        matchLoc = minLoc;
                    }
                    else
                        matchLoc = maxLoc;

                    rectangle(img_out, matchLoc, Point(matchLoc.x + templ.cols, matchLoc.y + templ.rows), Scalar(0, 0, 255), 1);

                    //cvtColor �Լ��� �̿��Ͽ� ��������� gray�� ����
                    cvtColor(result, result, COLOR_GRAY2BGR);

                    //��ǥ�� ������ �簢�� �� ��������
                    circle(result, matchLoc, 3, Scalar(0, 0, 255), 1);

                    // imshow �̹��� ��� �Լ�
                    imshow("src", img_out);
                    imshow("templ", templ);
                    imshow("result", result);

                    waitKey();
                }

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