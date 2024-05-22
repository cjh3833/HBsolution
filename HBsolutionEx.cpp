/*
* 2018848045 최재형 HB솔루션
*/

// pylon API를 사용하기 위한 헤더 파일 포함.
#include <pylon/PylonIncludes.h>
// openCV를 사용하기 위한 헤더파일
#include <opencv2/opencv.hpp>
//time을 쓰기 위한 헤더파일
#include <time.h>
#include <windows.h>
#include <iostream>
#include <iomanip> // 10진수 -> 16진수
#include <chrono>
#include <thread>
#include "HBsolHistogram.h"


#ifdef PYLON_WIN_BUILD
#    include <pylon/PylonGUI.h> // Windows 환경에서 GUI 관련 기능을 사용하기 위한 헤더 파일.
#endif

#define Optimal 0.7

// pylon 객체 사용을 위한 네임스페이스.
// cv 객체 사용을 위한 네임스페이스
// std 사용을 위한 네임스페이스
using namespace Pylon;
using namespace cv;
using namespace std;

// 캡처할 이미지의 수를 정의.
static const uint32_t c_countOfImagesToGrab = 300;


int main(int /*argc*/, char* /*argv*/[])
{
    HANDLE hSerial;
    Point minLoc, maxLoc, matchLoc;
    Mat result;
    clock_t start, end;

    int method = 1, histsize = 256, range = 256, histogram[256] = { 0 };
    int  bright = 0;
    double minVal, maxVal;
    double threshold_max = 0.05;
    double threshold_min = 0.02;

    // pylon 메소드를 사용하기 전에 pylon 런타임을 초기화해야 함.
    PylonInitialize();

    //템블릿 이미지 저장
    Mat templ = imread("templ_04_10.png", cv::IMREAD_GRAYSCALE); //templ용
    if (templ.empty()) {
        std::cerr << "templ 이미지 없음" << std::endl;
        return 1;
    }

    Mat src_compare = imread("compare_src.png", cv::IMREAD_GRAYSCALE); //compare용
    if (src_compare.empty()) {
        std::cerr << "compare 이미지 없음" << std::endl;
        return 1;
    }
    try
    {
        // 첫 번째로 찾은 카메라 장치로 인스턴트 카메라 객체 생성.
        CInstantCamera camera(CTlFactory::GetInstance().CreateFirstDevice());

        // MaxNumBuffer 파라미터는 캡처를 위해 할당된 버퍼의 수를 제어하는 데 사용될 수 있음.
        // 이 파라미터의 기본값은 10.
        camera.MaxNumBuffer = 5;
        camera.GetInstantCameraNodeMap();
        // c_countOfImagesToGrab 이미지의 캡처 시작.
        // 카메라 장치는 연속 취득을 설정하는 기본 구성으로 파라미터화됨.
        camera.StartGrabbing();

        // 이 스마트 포인터는 캡처 결과 데이터를 받게 됨.
        CGrabResultPtr ptrGrabResult;
        Mat test, src;
        Mat src2;
        double Similarity = 0;

        CImageFormatConverter formatConverter;
        formatConverter.OutputPixelFormat = PixelType_BGR8packed;
        CPylonImage pylonImage;

        OpenSerialPort(hSerial, L"COM4");  // 포트시리얼, 포트이름 설정

        // 시작하면 초기 밝기 0으로 맞춰주고 시작
        //int형이지만 저장안해서 밝기 0으로 시작가능
        Light_Controll_Bright(hSerial, bright, Similarity);

        // c_countOfImagesToGrab 이미지가 검색되었을 때 Camera.StopGrabbing()이 자동으로 호출됨.
        while (camera.IsGrabbing())
        {
            start = clock(); //시간체크 시작

            // 이미지를 기다린 다음 검색. 500ms의 타임아웃 사용.
            camera.RetrieveResult(500, ptrGrabResult, TimeoutHandling_ThrowException);

            // 이미지가 성공적으로 캡처되었는가?
            if (ptrGrabResult->GrabSucceeded())
            {
                // 이미지 데이터에 접근.
                const uint8_t* pImageBuffer = (uint8_t*)ptrGrabResult->GetBuffer();
                cout << "Gray value of first pixel: " << (uint32_t)pImageBuffer[0] << endl;

                // 이미지 데이터 변환 후 그레이 스케일 변환 작업
                // pylonImage에서 ptrGrabResult로 이미지 데이터를 변환하는 작업
                formatConverter.Convert(pylonImage, ptrGrabResult);

                // ptrGrabResult에서 가져온 이미지 데이터를 Mat형식으로 변환
                Mat src = cv::Mat(ptrGrabResult->GetHeight(), ptrGrabResult->GetWidth(), CV_8UC3, (uint8_t*)pylonImage.GetBuffer());

                //src이미지를 GRAY 스케일로 변환
                cv::cvtColor(src, src, COLOR_BGR2GRAY); //GRAY로 받아줌

                Mat img_out;
                src.copyTo(img_out); //src을 img_out으로 얕은 복사, 패턴매칭 하기위함

                // 원본 이미지에서 탬플릿 이미지와 일치하는 영역을 찾는 알고리즘
                matchTemplate(src, templ, result, method);
                // normalize를 이용해서 이미지 정규화, 필터의 종류, 0~1까지 분포
                normalize(result, result, 0, 1, NORM_MINMAX, -1, Mat());

                // 주어진 행력의 최소값, 최대값을 찾는 함수로 최소값, 최대값이 있는 좌표정보도 함께 알아낼 수 있음
                // Val -> 값 표시,     Loc -> 좌표표시
                minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc, Mat());

                //작동시간 체크 이후 GRAY로 변경, 그 뒤 작동시간 체크, geometric, masking기법 사용
                for (int i = 0; i < result.rows; i++) {
                    for (int j = 0; j < result.cols; j++) {
                        if (result.at<float>(i, j) >= threshold_min && result.at<float>(i, j) <= threshold_max) {
                            // 임계값 이상의 좌표에 template(pattern) 크기만큼을 더해 사각형을 그림.
                            // OpenCV의 경우 RGB 순서가 아닌 BGR 순서로 표시함.
                            //rectangle(img_out, Point(j, i), Point(j + templ.cols, i + templ.rows), Scalar(0, 0, 255), 1);
                            histogram[(int)img_out.at<uchar>(i, j)]++;
                        }
                    }
                }

                Mat src_hist, src_hist_result, templ_hist, templ_hist_img;
                Mat src_compare_hist, src_compare_result;
                Mat threshold_img, threshold_hist, threshold_hist_result; // 히스토그램의 이진화

                //calc_Histo(src, src_hist, histsize, range); // 히스토그램 계산
                //draw_histo(src_hist, src_hist_result); //히스토그램 그래프 그리기

                //calc_Histo(src_compare, src_compare_hist, histsize, range); // 히스토그램 계산
                //draw_histo(src_compare_hist, src_compare_result); //히스토그램 그래프 그리기

                //위 코드 함수, 히스토그램 계산 + 그리기
                create_hist(src, src_hist, src_hist_result);
                create_hist(src_compare, src_compare_hist, src_compare_result);

                /*int x = 0, y = 0;
                int pixelvalue = src.at<uchar>(y, x);

                std::cout << "pixel value at (x,y) : " << pixelvalue << std::endl;
                */

                //Similarity = cv::compareHist(src_hist, src_compare_hist, HISTCMP_CORREL);
                //cout << "Similarity : " << Similarity << endl;


                if (Similarity < Optimal) {
                    Similarity = cv::compareHist(src_hist, src_compare_hist, HISTCMP_CORREL);

                    bright = Light_Controll_Bright(hSerial, bright, Similarity);
                    cout << "Similarity : " << Similarity << endl;
                }

                else {
                    Bright(bright);
                    cout << "Similarity : " << Similarity << endl;
                }

                //    // 커맨드 전송 예제
                //    
                //    BYTE commandC[] = { 0x43, 0x31, static_cast<BYTE>(bright) };  // 채널 1, 데이터 250, 출력 ON
                //    if (!WriteToSerialPort(hSerial, commandC, sizeof(commandC))) {
                //        std::cerr << "Error write to serial port: " << GetLastError() << std::endl;
                //        exit(1);
                //    } std::cerr << "Success Write to serial port!  " << "Bright: " << bright << endl;
                //    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 밝기 변화 시간 간격 ms
                //    bright++;
                //    //  if (i >= value) { break; } // 임계값에 다다르면 밝기 변화 정지
                //}

                //create_hist(threshold_img, threshold_hist, threshold_hist_result); //마스킹된 이미지 그리기

                // 히스토그램 2개 compareHist()로 비교해보기
                // int method 
                // 1, cv2.HISTCMP_CORREL : 상관관계
                // 1 : 완전일치, -1 : 완전 불일치, 0 : 무관계
                // 빠르지만 부정확
                // 
                // 2. cv2.HISTCMP_CHISQR : 카이제곱 검정
                // 0 : 완전 일치, 무한대 : 완전 불일치
                // 
                // 3. cvHISTCMP_INTERSECT : 교차
                // 1 : 완전 일치, 0: 완전 불일치(히스토그램이 1로 정규화된 경우)
                // 
                // 4. cv2.HISTCMP_BHATTACHARYYA " 밭타차야 거리
                // 0 : 완전 일치, 1: 완전 불일치
                // 느리지만 가장 정확
                // 
                // 5. EMD
                // 직관적이지만 가장 느림
                // 

                //출력
                imshow("templ", templ);
                imshow("result", result);
                imshow("hist", src_hist_result);
                imshow("Compare Histogram", src_compare_result);
                imshow("compare_src", src_compare);
                imshow("src", img_out);

                end = clock();

                cout << "소요시간 : " << difftime(end, start) / CLOCKS_PER_SEC << endl;

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
        int exitCode = 1;
    }

    // 종료 전에 대기를 비활성화하려면 다음 두 줄을 주석 처리.
    cerr << endl << "Press enter to exit." << endl;
    while (cin.get() != '\n');

    // 모든 pylon 리소스를 해제.
    //PylonTerminate();

    return 1;
}