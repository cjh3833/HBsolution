/* 
*  HB�ַ��
*  multi pattern matching 4��8��
*  histogram �� ��� ���� 4�� 9��
*  ������׷� GRAY ���� 4�� 10��
*/

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

//������׷� ��� �Լ�
void calc_Histo(const Mat& image, Mat& hist, int bins, int range_max = 256)
{
    int histSize[] = { bins }; // ������׷� ��� ����
    float range[] = { 0, (float)range_max }; // 0�� ä�� ȭ�Ұ� ����
    int channels[] = { 0 }; //ä�� ��� - ���� ä��
    const float* ranges[] = { range }; //��� ä�� ȭ�� ����

    calcHist(&image, 1, channels, Mat(), hist, 1, histSize, ranges);
}

//������׷� ����� �Լ�
void draw_histo(Mat hist, Mat& hist_img, Size size = Size(256,200)) {
    hist_img = Mat(size, CV_8U, Scalar(255)); //�׷��� ���
    float bin = (float)hist_img.cols / hist.rows; // �� ��� �ʺ�
    normalize(hist, hist, 0, hist_img.rows, NORM_MINMAX);

    for (int i = 0; i < hist.rows; i++)
    {
        float start_x = i * bin; // ���� �簢�� ���� X ��ǥ
        float end_x = (i + 1) * bin; // ���� �簢�� ���� x ��ǥ
        Point2f pt1(start_x, 0);
        Point2f pt2(end_x, hist.at<float>(i));

        if (pt2.y > 0)
            rectangle(hist_img, pt1, pt2, Scalar(0), -1); // ���� �簢�� �׸���
    }
    flip(hist_img, hist_img, 0); // x�� ���� ���� ������
}

int main(int /*argc*/, char* /*argv*/[])
{
    // �̹��� ��� ������ ������ ����
    double minVal, maxVal;
    Point minLoc, maxLoc;
    Point matchLoc;
    Mat result;
    int method = 1;
    clock_t start, end;

    //������׷�
    int histogram[256] = { 0 };

    //�Ӱ谪 ����
    double threshold_max = 0.05;
    double threshold_min = 0.02;

    //�ð� ����� ������ ��
    int time_watch = 0;
    double time_working = 0.0;

    // ���� ���ø����̼��� ���� �ڵ�.
    int exitCode = 0;

    // pylon �޼ҵ带 ����ϱ� ���� pylon ��Ÿ���� �ʱ�ȭ�ؾ� ��.
    PylonInitialize();

    //�ۺ� �̹��� ����
    Mat templ = imread("templ_04_10.png", cv::IMREAD_GRAYSCALE);
    //Mat templ = imread("templ_4_1.png");
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
                // �̹��� �����Ϳ� ����.
                const uint8_t* pImageBuffer = (uint8_t*)ptrGrabResult->GetBuffer();
                cout << "SizeX: " << ptrGrabResult->GetWidth() << endl;
                cout << "SizeY: " << ptrGrabResult->GetHeight() << endl;
                cout << "Gray value of first pixel: " << (uint32_t)pImageBuffer[0] << endl;
                cout << "time : " << time_watch << endl;

                //�̹��� �޾ƿ��� src����ȯ �� imshow
                //gray�� ��ȯ���־��⶧���� CV_8UC3���� ����
                //src = cv::Mat(ptrGrabResult->GetHeight(), ptrGrabResult->GetWidth(), CV_8UC3, (uint8_t*)ptrGrabResult->GetBuffer());

                // �� ?
                formatConverter.Convert(pylonImage, ptrGrabResult);
                Mat src = cv::Mat(ptrGrabResult->GetHeight(), ptrGrabResult->GetWidth(), CV_8UC3, (uint8_t*)pylonImage.GetBuffer());
                cv::cvtColor(src, src, COLOR_BGR2GRAY);

                Mat img_out;
                src.copyTo(img_out);

                /* ��Ī ���
                0: TM_SQDIFF //��ġ�ϸ� �Ҽ��� ���� �۾���
                1: TM_SQDIFF NORMED //��ġ�ϸ� �Ҽ��� ���� �۾���
                2: TM CCORR
                3: TM CCORR NORMED
                4: TM COEFF
                5: TM COEFF NORMED
                */

                start = clock();

                // ���� �̹������� ���ø� �̹����� ��ġ�ϴ� ������ ã�� �˰���
                matchTemplate(src, templ, result, method);
                // normalize�� �̿��ؼ� �̹��� ����ȭ, ������ ����, 0~1���� ����
                normalize(result, result, 0, 1, NORM_MINMAX, -1, Mat());

                // �־��� ����� �ּҰ�, �ִ밪�� ã�� �Լ��� �ּҰ�, �ִ밪�� �ִ� ��ǥ������ �Բ� �˾Ƴ� �� ����
                //Val -> �� ǥ��,     Loc -> ��ǥǥ��
                minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc, Mat());

                //�۵��ð� üũ ���� GRAY�� ����, �� �� �۵��ð� üũ, geometric, masking��� ���
                for (int i = 0; i < result.rows; i++) {
                    for (int j = 0; j < result.cols; j++) {
                        if (result.at<float>(i, j) >= threshold_min && result.at<float>(i, j) <= threshold_max) {
                            // �Ӱ谪 �̻��� ��ǥ�� template(pattern) ũ�⸸ŭ�� ���� �簢���� �׸�.
                            // OpenCV�� ��� RGB ������ �ƴ� BGR ������ ǥ����.
                            rectangle(img_out, Point(j, i), Point(j + templ.cols, i + templ.rows), Scalar(0, 0, 255), 1);
                            histogram[(int)img_out.at<uchar>(i, j)]++;

                        }
                    }
                }

                end = clock();
                double searching_time = difftime(end, start) / CLOCKS_PER_SEC;

                Mat hist, hist_img;
                calc_Histo(src, hist, 256); //������׷� ����Լ�
                draw_histo(hist, hist_img); // ������׷� �׷��� �����

                //drawHist(histogram);

                cout << "����ð� : " << searching_time << endl << endl;

                // imshow �̹��� ��� �Լ�
                imshow("hist_img", hist_img);
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