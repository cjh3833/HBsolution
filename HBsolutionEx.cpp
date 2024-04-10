/* abort has been called
*  HB솔루션
*  multi pattern matching 4월8일
*  histogram 색 출력 구현 4월 9일
*  히스토그램 GRAY 구현 4월 10일
*/

// pylon API를 사용하기 위한 헤더 파일 포함.
#include <pylon/PylonIncludes.h>
// openCV를 사용하기 위한 헤더파일
#include <opencv2/opencv.hpp> 
//time을 쓰기 위한 헤더파일
#include <time.h>

#ifdef PYLON_WIN_BUILD
#    include <pylon/PylonGUI.h> // Windows 환경에서 GUI 관련 기능을 사용하기 위한 헤더 파일.
#endif


// pylon 객체 사용을 위한 네임스페이스.
// cout 사용을 위한 네임스페이스.
using namespace Pylon;
// cv 객체 사용을 위한 네임스페이스
// std 사용을 위한 네임스페이스
using namespace cv;
using namespace std;

// 캡처할 이미지의 수를 정의.
static const uint32_t c_countOfImagesToGrab = 300;

//히스토그램 계산 함수
void calc_Histo(const Mat& image, Mat& hist, int bins, int range_max = 256)
{
    int histSize[] = { bins }; // 히스토그램 계급 개수
    float range[] = { 0, (float)range_max }; // 0번 채널 화소값 범위
    int channels[] = { 0 }; //채널 목록 - 단일 채널
    const float* ranges[] = { range }; //모든 채널 화소 범위

    calcHist(&image, 1, channels, Mat(), hist, 1, histSize, ranges);
}

//히스토그램 드로잉 함수
void draw_histo(Mat hist, Mat& hist_img, Size size = Size(256, 200)) {
    hist_img = Mat(size, CV_8U, Scalar(255)); //그래프 행렬
    float bin = (float)hist_img.cols / hist.rows; // 한 계급 너비
    normalize(hist, hist, 0, hist_img.rows, NORM_MINMAX);

    for (int i = 0; i < hist.rows; i++)
    {
        float start_x = i * bin; // 막대 사각형 시작 X 좌표
        float end_x = (i + 1) * bin; // 막대 사각형 종료 x 좌표
        Point2f pt1(start_x, 0);
        Point2f pt2(end_x, hist.at<float>(i));

        if (pt2.y > 0)
            rectangle(hist_img, pt1, pt2, Scalar(0), -1); // 막대 사각형 그리기
    }
    flip(hist_img, hist_img, 0); // x축 기준 영상 뒤집기
}

void create_hist(Mat img, Mat& hist, Mat& hist_img)
{
    int histsize = 256, range = 256;
    calc_Histo(img, hist, histsize, range); // 히스토그램 계산
    draw_histo(hist, hist_img); //히스토그램 그래프 그리기
}

int main(int /*argc*/, char* /*argv*/[])
{
    // 이미지 결과 연산을 시켜줄 변수
    double minVal, maxVal;
    Point minLoc, maxLoc;
    Point matchLoc;
    Mat result;
    int method = 1;
    clock_t start, end;

    //히스토그램
    int histogram[256] = { 0 };

    //임계값 설정
    double threshold_max = 0.05;
    double threshold_min = 0.02;

    //시간 경과를 보여줄 것
    int time_watch = 0;
    double time_working = 0.0;

    // 샘플 애플리케이션의 종료 코드.
    int exitCode = 0;

    // pylon 메소드를 사용하기 전에 pylon 런타임을 초기화해야 함.
    PylonInitialize();

    //템블릿 이미지 저장
    Mat templ = imread("templ_04_10.png", cv::IMREAD_GRAYSCALE);
    //Mat templ = imread("templ_4_1.png");
    Mat templ2 = imread("templ_5.png");

    try
    {
        // 첫 번째로 찾은 카메라 장치로 인스턴트 카메라 객체 생성.
        CInstantCamera camera(CTlFactory::GetInstance().CreateFirstDevice());

        // 카메라의 모델 이름 출력.
        cout << "Using device " << camera.GetDeviceInfo().GetModelName() << endl;

        // MaxNumBuffer 파라미터는 캡처를 위해 할당된 버퍼의 수를 제어하는 데 사용될 수 있음.
        // 이 파라미터의 기본값은 10.
        camera.MaxNumBuffer = 5;

        // c_countOfImagesToGrab 이미지의 캡처 시작.
        // 카메라 장치는 연속 취득을 설정하는 기본 구성으로 파라미터화됨.
        camera.StartGrabbing(c_countOfImagesToGrab);

        // 이 스마트 포인터는 캡처 결과 데이터를 받게 됨.
        CGrabResultPtr ptrGrabResult;  //캡쳐결과를 받으면 이걸 패턴매칭에 연결시키면 ?
        Mat test, src;
        Mat test2, src2;

        // 왜 ?
        CImageFormatConverter formatConverter;
        formatConverter.OutputPixelFormat = PixelType_BGR8packed;
        CPylonImage pylonImage;

        // c_countOfImagesToGrab 이미지가 검색되었을 때 Camera.StopGrabbing()이 자동으로 호출됨.
        while (camera.IsGrabbing())
        {
            //시간경과를 보여주기 위해 증가
            time_watch++;

            // 이미지를 기다린 다음 검색. 5000ms의 타임아웃 사용.
            camera.RetrieveResult(5000, ptrGrabResult, TimeoutHandling_ThrowException);

            // 이미지가 성공적으로 캡처되었는가?
            if (ptrGrabResult->GrabSucceeded())
            {
                // 이미지 데이터에 접근.
                const uint8_t* pImageBuffer = (uint8_t*)ptrGrabResult->GetBuffer();
                cout << "SizeX: " << ptrGrabResult->GetWidth() << endl;
                cout << "SizeY: " << ptrGrabResult->GetHeight() << endl;
                cout << "Gray value of first pixel: " << (uint32_t)pImageBuffer[0] << endl;
                cout << "time : " << time_watch << endl;

                //이미지 받아오고 src형변환 후 imshow
                //gray로 변환해주었기때문에 CV_8UC3으로 변경
                //src = cv::Mat(ptrGrabResult->GetHeight(), ptrGrabResult->GetWidth(), CV_8UC3, (uint8_t*)ptrGrabResult->GetBuffer());

                // 왜 ?
                formatConverter.Convert(pylonImage, ptrGrabResult);
                Mat src = cv::Mat(ptrGrabResult->GetHeight(), ptrGrabResult->GetWidth(), CV_8UC3, (uint8_t*)pylonImage.GetBuffer());
                cv::cvtColor(src, src, COLOR_BGR2GRAY);

                Mat img_out;
                src.copyTo(img_out);
                /* 매칭 방법
                0: TM_SQDIFF //일치하면 할수록 값이 작아짐
                1: TM_SQDIFF NORMED //일치하면 할수록 값이 작아짐
                2: TM CCORR
                3: TM CCORR NORMED
                4: TM COEFF
                5: TM COEFF NORMED
                */

                start = clock();

                // 원본 이미지에서 탬플릿 이미지와 일치하는 영역을 찾는 알고리즘
                matchTemplate(src, templ, result, method);
                // normalize를 이용해서 이미지 정규화, 필터의 종류, 0~1까지 분포
                normalize(result, result, 0, 1, NORM_MINMAX, -1, Mat());

                // 주어진 행력의 최소값, 최대값을 찾는 함수로 최소값, 최대값이 있는 좌표정보도 함께 알아낼 수 있음
                //Val -> 값 표시,     Loc -> 좌표표시
                minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc, Mat());

                //작동시간 체크 이후 GRAY로 변경, 그 뒤 작동시간 체크, geometric, masking기법 사용
                for (int i = 0; i < result.rows; i++) {
                    for (int j = 0; j < result.cols; j++) {
                        if (result.at<float>(i, j) >= threshold_min && result.at<float>(i, j) <= threshold_max) {
                            // 임계값 이상의 좌표에 template(pattern) 크기만큼을 더해 사각형을 그림.
                            // OpenCV의 경우 RGB 순서가 아닌 BGR 순서로 표시함.
                            rectangle(img_out, Point(j, i), Point(j + templ.cols, i + templ.rows), Scalar(0, 0, 255), 1);
                            histogram[(int)img_out.at<uchar>(i, j)]++;

                        }
                    }
                }

                end = clock();
                double searching_time = difftime(end, start) / CLOCKS_PER_SEC;

                Mat dst1, dst2, hist_img1, hist_img2;
                
                //되는것
                Mat hist, hist_img;

                create_hist(src, hist, hist_img); // 히스토그램 및 그래프 그리기

                ////히스토그램 누적합 계산
                //Mat accum_hist = Mat(hist.size(), hist.type(), Scalar(0));
                //accum_hist.at<float>(0) = hist.at<float>(0);

                //for (int i = i; i < hist.rows; i++) {
                //    accum_hist.at<float>(i) = accum_hist.at<float>(i - 1) + hist.at<float>(i);
                //}

                //accum_hist /= sum(hist)[0];
                //accum_hist *= 255;
                //dst1 = Mat(src.size(), CV_8U);
                ////중복연산 체크 ㄲ
                //for (int i = 0; i < src.rows; i++) {
                //    for (int j = 0; j < src.cols; j++)
                //    {
                //        int idx = src.at<uchar>(i, j);
                //        dst1.at<uchar>(i, j) = (uchar)accum_hist.at<float>(idx);
                //    }
                //}

                //equalizeHist(src, dst2);
                //create_hist(dst1, hist, hist_img1);
                //create_hist(dst2, hist, hist_img2);
                //
                ////히스토그램 GRAY 출력
                ////create_hist(src, hist, hist_img);

                //cout << "연산시간 : " << searching_time << endl << endl;

                //// imshow 이미지 출력 함수
                //imshow("dst1-User", dst1); 
                //imshow("User_hist", hist_img1); // 사용자 평활화
                //imshow("dst2-OpenCV", dst2); 
                //imshow("OpenCV_hist", hist_img2); // OpenCV 평활화

                imshow("hist", hist_img);
                imshow("src", img_out);
                imshow("templ", templ);
                imshow("result", result);



                waitKey(1);


#ifdef PYLON_WIN_BUILD
                // 캡처된 이미지를 표시.
                //Pylon::DisplayImage(1, ptrGrabResult);
#endif
            }
            else
            {
                // 오류 메시지 출력.
                cout << "Error 쵸비상: " << std::hex << ptrGrabResult->GetErrorCode() << std::dec << " " << ptrGrabResult->GetErrorDescription() << endl;
            }
        }
    }
    catch (const GenericException& e)
    {
        // 예외 처리.
        cerr << "An exception occurred." << endl
            << e.GetDescription() << endl;
        exitCode = 1;
    }

    // 종료 전에 대기를 비활성화하려면 다음 두 줄을 주석 처리.
    cerr << endl << "Press enter to exit." << endl;
    while (cin.get() != '\n');

    // 모든 pylon 리소스를 해제.
    PylonTerminate();

    return exitCode;
}