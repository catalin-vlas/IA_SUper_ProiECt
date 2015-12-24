#include <QCoreApplication>
#include <iostream>
#include <fstream>
#include <QImage>
#include <QColor>
#include <QString>
#include <cstring>
#include <vector>
#include <QDir>
#include <map>
#include <cmath>
#include "QFileInfo"

#define LEARNING_PHASE
//#define PROCESS_PHASE
using namespace std;

int class_identifier[255];
string emotion_list[10]={"terror","fear","giref","sadness","serenity","joy","ecstasy","surprise","amazement","distraction"};

typedef struct bucket{
    int rgb[4][4][4];
    int nrpixels;

    bucket() {
        memset(rgb,0,sizeof(rgb));
        nrpixels=0;
    }

    void calcPercent(){

        if (nrpixels>0)
        for (int i=0; i<4; ++i)
             for (int j=0; j<4; ++j)
                  for (int k=0; k<4; ++k)
                      rgb[i][j][k]=(rgb[i][j][k]*100)/nrpixels;

    }

} bucket;

class imageInfo{
public:
    vector<bucket> term;
    int width;
    int height;
    int nr_of_segm;
    QString path;
    string category;

    imageInfo(QString path,int nr_of_segm)
    {
        QImage img(path);

        this->nr_of_segm=nr_of_segm;
        this->height=img.height();
        this->width=img.width();
        this->path=path;

        int segment_length=(height*width)/nr_of_segm;
        int step=1;
        bucket currentBucket;

        for ( int row = 0; row < img.height(); ++row )
            for ( int col = 0; col < img.width(); ++col )
            {
                QColor clrCurrent( img.pixel( col, row ) );

                int r=class_identifier[clrCurrent.red()];
                int g=class_identifier[clrCurrent.green()];
                int b=class_identifier[clrCurrent.blue()];

                ++currentBucket.rgb[r][g][b];
                ++currentBucket.nrpixels;

                if (step%segment_length==0) {

                    currentBucket.calcPercent();

                    term.push_back(currentBucket);

                    memset(currentBucket.rgb,0,sizeof(currentBucket.rgb));
                    currentBucket.nrpixels=0;

                }

                ++step;

            }

         }

    imageInfo(QString path){

        ifstream fin(path.toStdString());

        fin>>nr_of_segm;


        for (int i=0; i<nr_of_segm; ++i)
        {
            bucket current_bucket;

            for (int j=0; j<4; ++j)
                 for (int k=0; k<4; ++k)
                     for (int t=0; t<4; ++t)
                     {
                         fin>>current_bucket.rgb[j][k][t];
                        //if (current_bucket.rgb[j][k][t]>0) cout<<current_bucket.rgb[j][k][t]<<"\n";
                     }

            term.push_back(current_bucket);
        }

    }

    void saveImg(string path)
    {

        ofstream fout(path);

        fout<<term.size()<<"\n";

        for (unsigned int i=0; i<term.size(); ++i)
            for (int j=0; j<4; ++j)
                 for (int k=0; k<4; ++k)
                     for (int t=0; t<4; ++t)
                         fout<<term[i].rgb[j][k][t]<<"\n";

    }

    void setCateg(string path)
    {
        ifstream fin(path);

        string content;
        getline(fin,content,char(EOF));

        for (int i=0; i<10; ++i)
            if ( content.find(emotion_list[i])!=string::npos)
            {
                category=emotion_list[i];
                break;
            }
    }
};

class img_classes{
public:
    map<string, map<int, vector<bucket> > > cont;

    img_classes()
    {
        cont.clear();
    }

    void update_class(imageInfo img)
    {
        for (int i=0; i<img.nr_of_segm; ++i)
            cont[img.category][i].push_back(img.term[i]);
    }

private:
    double dist(bucket b1, bucket b2)
    {
        double rez=0;

        for (int i=0; i<4; ++i)
             for (int j=0; j<4; ++j)
                 for (int k=0; k<4; ++k)
                     rez+=(b1.rgb[i][j][k]-b2.rgb[i][j][k])*(b1.rgb[i][j][k]-b2.rgb[i][j][k]);

        return sqrt(rez);
    }

    double d_s_dist(bucket b1, vector<bucket> v)//distance between a descriptor and a list of neibhors
    {
        double rez=1000000000;

        for (unsigned int i=0; i<v.size(); ++i)
            rez=min(rez,dist(v[i],b1));

        return rez;
    }

    double getDistance(map<int, vector<bucket> > class_identifier, imageInfo img)
    {
        map<int, vector<bucket> >::iterator it;

        double rez=0;

        for (it=class_identifier.begin(); it!=class_identifier.end(); ++it)
            rez+=d_s_dist(img.term[it->first],it->second);

        return rez;

    }

public:
    string getClass(imageInfo img)
    {
        map<string, map<int, vector<bucket> > >::iterator it=cont.begin();

        ++it;

        double min_dist=getDistance(it->second,img);
        string predicted_class=it->first;

        cout<<min_dist<<" "<<it->first<<"\n";

        ++it;

        while (it!=cont.end()){

            double curr_dist=getDistance(it->second,img);

            cout<<curr_dist<<" "<<it->first<<"\n";

            if (curr_dist<min_dist)
            {
                min_dist=curr_dist;
                predicted_class=it->first;
            }

            ++it;
        }

        return predicted_class;
    }

};

int main()
{

    for (int i=0, k=-1; i<=255; ++i)
    {
        if (i%64==0) ++k;
        class_identifier[i]=k;
    }

    /* build hist info and save info files */
#ifdef PROCESS_PHASE
    QStringList filters;
    filters<<"*.jpg"<<"*.jpeg"<<"*.bmp"<<"*.png";

    QString rootpath="C:\\Users\\vlas\\Desktop\\IA_Image_ALL";

    QDir image_dir(rootpath);
    image_dir.setNameFilters(filters);

    QStringList all_img=image_dir.entryList();

    for (int i=0; i<all_img.size(); ++i) {

        imageInfo my_img(rootpath+"\\"+all_img.at(i),16);
        cout<<all_img.at(i).toStdString()<<"\n";

        my_img.saveImg(rootpath.toStdString()+"\\"+(all_img.at(i)).toStdString()+".hist");

    }

    cout<<"done\n";
#endif

    /*build image info from info provided by histogram descriptions and also get image category and update clases*/
#ifdef LEARNING_PHASE

    QStringList filters;
    filters<<"*.hist";

    QString rootpath="C:\\Users\\vlas\\Desktop\\IA_Image_ALL";

    QDir image_dir(rootpath);
    image_dir.setNameFilters(filters);

    int counter=0;

    QStringList all_img=image_dir.entryList();

    img_classes class_builder;

    for (int i=0; i<all_img.size(); ++i) {

        imageInfo my_img(rootpath+"\\"+all_img.at(i));

        string image_xml=rootpath.toStdString()+"\\";

        for (int j=0; all_img.at(i)[j]!='.'; ++j) image_xml+=all_img.at(i).toStdString()[j];

        image_xml+=".xml";
        my_img.setCateg(image_xml);

        class_builder.update_class(my_img);

        cout<<all_img.at(i).toStdString()<<"\n";

        ++counter;

    }

    cout<<"Classes done\n";
    cout<<counter<<" images was processed\n";

    cout<<"Introduce the path to an image to predict:\n";

    string path="";

    getline(cin,path);

    while (path!="exit"){

        cout<<path<<"\n";

        QFileInfo checkFile(QString::fromStdString(path));

        if (checkFile.exists() && checkFile.isFile()) {
             cout<<"File OK\n";
        }
        else
        {
             cout<<"File not found, try again\n";
             getline(cin,path);
             continue;
        }

        imageInfo img(QString::fromStdString(path),16);

        cout<<class_builder.getClass(img)<<"\n";

        getline(cin,path);
    }

#endif

  return 0;
}
