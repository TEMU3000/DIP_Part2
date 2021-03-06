#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "opencv2/objdetect.hpp"
#include "opencv2/videoio.hpp"
#include <iostream>
#include <time.h>
#include <string>
#include <stdlib.h>
#include <stdio.h>

using namespace cv;
using namespace std;

String face_cascade_name = "haarcascade_frontalface_alt.xml";
CascadeClassifier face_cascade;

int player_x = 200;
int wide = 480;

Mat detectAndMask( Mat frame );
void overlayImage(const Mat &background, const Mat &foreground, Mat &output, Point2i location);

Mat mask;
Mat ship;

int main(int argc, char *argv[])
{
    Mat bg = imread("bg.png", IMREAD_UNCHANGED);
    Mat met = imread("meteor.png", IMREAD_UNCHANGED);
    ship = imread("ship.png",IMREAD_UNCHANGED);
    mask = imread("mask.jpg",IMREAD_GRAYSCALE);

    namedWindow( "Scene", CV_WINDOW_AUTOSIZE );

    if( !face_cascade.load( face_cascade_name ) ){ printf("--(!)Error loading face cascade\n"); return -1; };

    VideoCapture capture(0);
    Mat frame;
    if ( ! capture.isOpened() ) { printf("--(!)Error opening video capture\n"); return -1; }
    int bgy = bg.rows;
    int mety = -108;
    int metx = 30;
    bool has_met = false;
    int score = 0;
    srand(time(NULL));
    int player2_x = 250;
    bool player2_alive = true;
    Mat player2 = imread("player2.png", IMREAD_UNCHANGED);

    if ( capture.read(frame) ) {
        if( frame.empty() ) {
            printf(" --(!) No captured frame -- Break!");
            return -1;
        }
        Mat startSceneBg;
        bg.copyTo(startSceneBg);
        Mat startScene = startSceneBg(Rect(0,bgy-720-1,frame.cols,720));
        putText(startScene, "Move you face left and right!", Point(120,360), FONT_HERSHEY_DUPLEX , 0.8, Scalar(0,0,255));

        imshow("Scene", startScene);
        waitKey(3000);
    }

    while ( capture.read(frame) ) {
        if( frame.empty() ) {
            printf(" --(!) No captured frame -- Break!");
            break;
        }

        Mat player = detectAndMask( frame );

        Mat scene = bg(Rect(0,bgy-720-1,frame.cols,720)); // x,y,width,height
        Mat new_scene;

        if(player2_alive){
            player2_x = player2_x + rand()%81 - 40;
            if(player2_x < 0){
                player2_x = 0;
            }
            if(player2_x > frame.cols - player2.cols){
                player2_x = frame.cols - player2.cols;
            }
            overlayImage(scene, player2, new_scene,Point(player2_x, 740 - player2.rows));
        }

        int player_x_screen = player_x * frame.cols / wide - player.cols/2;
        overlayImage((player2_alive) ? new_scene : scene, player, new_scene,Point(player_x_screen, 740 - player.rows));

        if(!has_met){
            //metx = (rand()%3 * 230 + 30); // 3-way
            metx = (rand()%(frame.cols-120)); // random
            overlayImage(new_scene, met, new_scene,Point(metx,-108));
            has_met = true;
        } else {
            if(mety >= 400 && mety < 660){
                if(abs((player_x_screen + player.cols/2) - (metx + 60) ) < 110){
                    Mat exp1 = imread("explosion.png", IMREAD_UNCHANGED);
                    Mat exp2 = imread("explosion2.png", IMREAD_UNCHANGED);
                    int exp_x = (player_x_screen + metx) / 2;
                    int exp_y = mety;
                    Mat ex_scene;
                    overlayImage(new_scene, exp1, ex_scene,Point(exp_x,exp_y));
                    imshow("Scene", ex_scene);
                    waitKey(300);
                    overlayImage(new_scene, exp2, ex_scene,Point(exp_x,exp_y));
                    imshow("Scene", ex_scene);
                    waitKey(300);
                    overlayImage(new_scene, exp1, ex_scene,Point(exp_x,exp_y));
                    imshow("Scene", ex_scene);
                    waitKey(300);
                    overlayImage(new_scene, exp2, ex_scene,Point(exp_x,exp_y));
                    imshow("Scene", ex_scene);
                    waitKey(300);
                    overlayImage(new_scene, exp1, ex_scene,Point(exp_x,exp_y));
                    imshow("Scene", ex_scene);
                    waitKey(300);
                    Mat gameover = frame;
                    string text = "Final Score: " + to_string(score);
                    putText(gameover, text, Point(200,frame.rows/2), FONT_HERSHEY_DUPLEX , 1, Scalar(0,0,255));

                    imshow("Scene", gameover);
                    waitKey(5000);
                    return score;
                }
                if(abs((player2_x + player2.cols/2) - (metx + 60) ) < 110){
                    player2_alive = false;
                }
            }
            if(mety >= 720){
                mety = -120;
                has_met = false;
                score++;
            } else {
                overlayImage(new_scene, met, new_scene,Point(metx,mety));
            }
        }

        string text = "Score: " + to_string(score);
        putText(new_scene, text, Point(20,40), FONT_HERSHEY_DUPLEX , 1, Scalar(0,0,255));

        imshow("Scene", new_scene);
        waitKey(40);
        bgy -= 16;
        if(bgy < 1280){
            bgy = bg.rows;
        }
        mety += 12 + score*2;
    }

    waitKey(0);
    return 0;
}

Mat detectAndMask( Mat frame )
{
    vector<Rect> faces;
    Mat frame_gray;
    Mat ship_copy;
    ship.copyTo(ship_copy);

    cvtColor( frame, frame_gray, COLOR_BGR2GRAY );

    equalizeHist( frame_gray, frame_gray );

    face_cascade.detectMultiScale( frame_gray, faces, 1.1, 2, 0|CASCADE_SCALE_IMAGE, Size(80, 80) );

    if(faces.size() == 0){
        return ship_copy;
    }

    Mat faceROI = frame( faces[0] );
    player_x = frame.cols - faces[0].x - faces[0].width;
    wide = frame.cols - faces[0].width;

    Mat re_mask;

    resize(mask,re_mask,Size(faceROI.cols,faceROI.rows));

    vector<Mat> channels;
    split(faceROI, channels);

    channels.push_back(re_mask);
    Mat result;
    merge(channels, result);
    resize(result,result,Size(100,100));

    Mat player;
    overlayImage(ship_copy, result, player,Point(26,68));

    return player;

}

void overlayImage(const Mat &background, const Mat &foreground, Mat &output, Point2i location){
    background.copyTo(output);

    for(int y = std::max(location.y , 0); y < background.rows; ++y) {
        int fY = y - location.y;

        if(fY >= foreground.rows)
            break;

        for(int x = std::max(location.x, 0); x < background.cols; ++x) {
            int fX = x - location.x;

            if(fX >= foreground.cols)
                break;

            double opacity = ((double)foreground.data[fY * foreground.step + fX * foreground.channels() + 3]) / 255.;

            for(int c = 0; opacity > 0 && c < output.channels(); ++c) {
                unsigned char foregroundPx = foreground.data[fY * foreground.step + fX * foreground.channels() + c];
                unsigned char backgroundPx = background.data[y * background.step + x * background.channels() + c];
                output.data[y*output.step + output.channels()*x + c] = backgroundPx * (1.-opacity) + foregroundPx * opacity;
            }
        }
    }
}



