#include <stdio.h>
#include <stdlib.h>
#include<math.h>

/*
 * マクロ定義
 */
#define min(A, B) ((A)<(B) ? (A) : (B))
#define max(A, B) ((A)>(B) ? (A) : (B))

/*
 * 画像構造体の定義
 */
typedef struct
{
    int width;              /* 画像の横方向の画素数 */
    int height;             /* 画像の縦方向の画素数 */
    int maxValue;           /* 画素の値(明るさ)の最大値 */
    unsigned char *data;    /* 画像の画素値データを格納する領域を指す */
                            /* ポインタ */
} image_t;


/*======================================================================
 * このプログラムに与えられた引数の解析
 *======================================================================
 */
void
parseArg(int argc, char **argv, FILE **infp, FILE **outfp)
{     
    FILE *fp;

    /* 引数の個数をチェック */
    if (argc!=3)
    {
        goto usage;
    }

    *infp = fopen(argv[1], "rb"); /* 入力画像ファイルをバイナリモードで */
                                /* オープン */

    if (*infp==NULL)		/* オープンできない時はエラー */
    {
        fputs("Opening the input file was failend\n", stderr);
        goto usage;
    }

    *outfp = fopen(argv[2], "wb"); /* 出力画像ファイルをバイナリモードで */
                                /* オープン */

    if (*outfp==NULL)		/* オープンできない時はエラー */
    {
        fputs("Opening the output file was failend\n", stderr);
        goto usage;
    }

    return;

/* このプログラムの使い方の説明 */
usage:
    fprintf(stderr, "usage : %s <input pgm file> <output pgm file>\n", argv[0]);
    exit(1);
}


/*======================================================================
 * 画像構造体の初期化
 *======================================================================
 * 画像構造体 image_t *ptImage の画素数(width × height)、階調数
 * (maxValue)を設定し、画素値データを格納するのに必要なメモリ領域を確
 * 保する。 
 */
void
initImage(image_t *ptImage, int width, int height, int maxValue)
{
    ptImage->width = width;
    ptImage->height = height;
    ptImage->maxValue = maxValue;

    /* メモリ領域の確保 */
    ptImage->data = (unsigned char *)malloc((size_t)(width * height));

    if (ptImage->data==NULL)    /* メモリ確保ができなかった時はエラー */
    {
        fputs("out of memory\n", stderr);
        exit(1);
    }
}


/*======================================================================
 * 文字列一行読み込み関数
 *======================================================================
 *   FILE *fp から、改行文字'\n'が表れるまで文字を読み込んで、char型の
 * メモリ領域 char *buf に格納する。1行の長さが n 文字以上の場合は、先
 * 頭から n-1 文字だけを読み込む。
 *   読み込んだ文字列の先頭が '#' の場合は、さらに次の行を読み込む。
 *   正常に読み込まれた場合は、ポインタ buf を返し、エラーや EOF (End
 * Of File) の場合は NULL を返す。
 */
char *
readOneLine(char *buf, int n, FILE *fp)
{
    char *fgetsResult;

    do
    {
        fgetsResult = fgets(buf, n, fp);
    } while(fgetsResult!=NULL && buf[0]=='#');
            /* エラーや EOF ではなく、かつ、先頭が '#' の時は、次の行 */
            /* を読み込む */

    return fgetsResult;
}   


/*======================================================================
 * PGM-RAW フォーマットのヘッダ部分の読み込みと画像構造体の初期化
 *======================================================================
 *   PGM-RAW フォーマットの画像データファイル FILE *fp から、ヘッダ部
 * 分を読み込んで、その画像の画素数、階調数を調べ、その情報に従って、
 * 画像構造体 image_t *ptImage を初期化する。
 *   画素値データを格納するメモリ領域も確保し、この領域の先頭を指すポ
 * インタを ptImage->data に格納する。
 *
 * !! 注意 !!
 *   この関数は、ほとんどの場合、正しく動作するが、PGM-RAWフォーマット
 * の正確な定義には従っておらず、正しいPGM-RAWフォーマットのファイルに
 * 対して、不正な動作をする可能性がある。なるべく、本関数をそのまま使
 * 用するのではなく、正しく書き直して利用せよ。
 */
void
readPgmRawHeader(FILE *fp, image_t *ptImage)
{
    int width, height, maxValue;
    char buf[128];

    /* マジックナンバー(P5) の確認 */
    if(readOneLine(buf, 128, fp)==NULL)
    {
        goto error;
    }
    if (buf[0]!='P' || buf[1]!='5')
    {
        goto error;
    }

    /* 画像サイズの読み込み */
    if (readOneLine(buf, 128, fp)==NULL)
    {
        goto error;
    }
    if (sscanf(buf, "%d %d", &width, &height) != 2)
    {
        goto error;
    }
    if ( width<=0 || height<=0)
    {
        goto error;
    }

    /* 最大画素値の読み込み */
    if (readOneLine(buf, 128, fp)==NULL)
    {
        goto error;
    }
    if (sscanf(buf, "%d", &maxValue) != 1)
    {
        goto error;
    }
    if ( maxValue<=0 || maxValue>=256 )
    {
        goto error;
    }

    /* 画像構造体の初期化 */
    initImage(ptImage, width, height, maxValue);

    return;

/* エラー処理 */
error:
    fputs("Reading PGM-RAW header was failed\n", stderr);
    exit(1);
}
     

/*======================================================================
 * PGM-RAWフォーマットの画素値データの読み込み
 *======================================================================
 *   入力ファイル FILE *fp から総画素数分の画素値データを読み込んで、
 * 画像構造体 image_t *ptImage の data メンバーが指す領域に格納する
 */
void
readPgmRawBitmapData(FILE *fp, image_t *ptImage)
{
    if( fread(ptImage->data, sizeof(unsigned char),
            ptImage->width * ptImage->height, fp)
            != ptImage->width * ptImage->height )
    {
        /* エラー */
        fputs("Reading PGM-RAW bitmap data was failed\n", stderr);
        exit(1);
    }
}

double valuecheck(double a)/*入力された値が0より小さかったら0を255より大きかったら255を返す関数*/
{
  double result;
  if(a<0){
    result = 0;
  }
  else if(a>255){
    result = 255;
  }
  else{
    result = a;
  }
  return result;
}

/*prewittフィルタを(2)の式で処理する。*/
void
filteringImage(image_t *resultImage, image_t *originalImage)
{
    int     x, y;
    int a[] = {-1,0,1,-1,0,1,-1,0,1};/*配列aにはprewittフィルタの水平成分を配置*/
    int b[] = {-1,-1,-1,0,0,0,1,1,1};/*配列bにはprewittフィルタの垂直成分を配置*/
    int     width, height;

    /* originalImage と resultImage のサイズが違う場合は、共通部分のみ */
    /* を処理する。*/
    width = min(originalImage->width, resultImage->width);
    height = min(originalImage->height, resultImage->height);

    for(y=1; y<height-1; y++)/*内側を処理する。*/
    {
        for(x=1; x<width-1; x++)
        {
          double dfdi = originalImage->data[x-1+originalImage->width*(y-1)]*a[0]
                        +originalImage->data[x+originalImage->width*(y-1)]*a[1]
                        +originalImage->data[x+1+originalImage->width*(y-1)]*a[2]
                        +originalImage->data[x-1+originalImage->width*y]*a[3]
                        +originalImage->data[x+originalImage->width*y]*a[4]
                        +originalImage->data[x+1+originalImage->width*y]*a[5]
                        +originalImage->data[x-1+originalImage->width*(y+1)]*a[6]
                        +originalImage->data[x+originalImage->width*(y+1)]*a[7]
                        +originalImage->data[x+1+originalImage->width*(y+1)]*a[8];
          double dfdj = originalImage->data[x-1+originalImage->width*(y-1)]*b[0]
                        +originalImage->data[x+originalImage->width*(y-1)]*b[1]
                        +originalImage->data[x+1+originalImage->width*(y-1)]*b[2]
                        +originalImage->data[x-1+originalImage->width*y]*b[3]
                        +originalImage->data[x+originalImage->width*y]*b[4]
                        +originalImage->data[x+1+originalImage->width*y]*b[5]
                        +originalImage->data[x-1+originalImage->width*(y+1)]*b[6]
                        +originalImage->data[x+originalImage->width*(y+1)]*b[7]
                        +originalImage->data[x+1+originalImage->width*(y+1)]*b[8];
          
          double result = sqrt(pow(dfdi,2)+pow(dfdj,2));/*新しい階級値を計算*/
          result = valuecheck(result);/*valuecheck関数で0~255に収まるようにする。*/
          resultImage->data[x+resultImage->width*y]=(unsigned char)result;/*unsigned charにもどして代入*/                             
        }
    }
    for(x=1;x<width-1;x++)/*上辺の処理*/
    {
      double dfdi = originalImage->data[x-1+originalImage->width*y]*a[3]
                    +originalImage->data[x+originalImage->width*y]*a[4]
                    +originalImage->data[x+1+originalImage->width*y]*a[5]
                    +originalImage->data[x-1+originalImage->width*(y+1)]*a[6]
                    +originalImage->data[x+originalImage->width*(y+1)]*a[7]
                    +originalImage->data[x+1+originalImage->width*(y+1)]*a[8];
      double dfdj = originalImage->data[x-1+originalImage->width*y]*b[3]
                    +originalImage->data[x+originalImage->width*y]*b[4]
                    +originalImage->data[x+1+originalImage->width*y]*b[5]
                    +originalImage->data[x-1+originalImage->width*(y+1)]*b[6]
                    +originalImage->data[x+originalImage->width*(y+1)]*b[7]
                    +originalImage->data[x+1+originalImage->width*(y+1)]*b[8];
      double result = sqrt(pow(dfdi,2)+pow(dfdj,2));
      result = valuecheck(result);
      resultImage->data[x]=(unsigned char)result;  
    }
    for(y=1;y<height-1;y++)/*右辺の処理*/
    {
      double dfdi = originalImage->data[x-1+originalImage->width*(y-1)]*a[0]
                    +originalImage->data[x+originalImage->width*(y-1)]*a[1]
                    +originalImage->data[x-1+originalImage->width*y]*a[3]
                    +originalImage->data[x+originalImage->width*y]*a[4]
                    +originalImage->data[x-1+originalImage->width*(y+1)]*a[6]
                    +originalImage->data[x+originalImage->width*(y+1)]*a[7];
      double dfdj = originalImage->data[x-1+originalImage->width*(y-1)]*b[0]
                    +originalImage->data[x+originalImage->width*(y-1)]*b[1]
                    +originalImage->data[x-1+originalImage->width*y]*b[3]
                    +originalImage->data[x+originalImage->width*y]*b[4]
                    +originalImage->data[x-1+originalImage->width*(y+1)]*b[6]
                    +originalImage->data[x+originalImage->width*(y+1)]*b[7];   
      double result = sqrt(pow(dfdi,2)+pow(dfdj,2));
      result = valuecheck(result);
      resultImage->data[resultImage->width*(y+1)-1]=(unsigned char)result;                
    }
    for(x=1;x<width-1;x++)/*下辺の処理*/
    {
      double dfdi = originalImage->data[x-1+originalImage->width*(y-1)]*a[0]
                        +originalImage->data[x+originalImage->width*(y-1)]*a[1]
                        +originalImage->data[x+1+originalImage->width*(y-1)]*a[2]
                        +originalImage->data[x-1+originalImage->width*y]*a[3]
                        +originalImage->data[x+originalImage->width*y]*a[4]
                        +originalImage->data[x+1+originalImage->width*y]*a[5];
                
      double dfdj = originalImage->data[x-1+originalImage->width*(y-1)]*b[0]
                        +originalImage->data[x+originalImage->width*(y-1)]*b[1]
                        +originalImage->data[x+1+originalImage->width*(y-1)]*b[2]
                        +originalImage->data[x-1+originalImage->width*y]*b[3]
                        +originalImage->data[x+originalImage->width*y]*b[4]
                        +originalImage->data[x+1+originalImage->width*y]*b[5];      
      double result = sqrt(pow(dfdi,2)+pow(dfdj,2));
      result = valuecheck(result);
      resultImage->data[x+resultImage->width*(resultImage->height-1)]=(unsigned char)result;            
    }
    for(y=1;y<height-1;y++)/*左辺の処理*/
    {
      double dfdi = originalImage->data[x+originalImage->width*(y-1)]*a[1]
                    +originalImage->data[x+1+originalImage->width*(y-1)]*a[2]
                    +originalImage->data[x+originalImage->width*y]*a[4]
                    +originalImage->data[x+1+originalImage->width*y]*a[5]   
                    +originalImage->data[x+originalImage->width*(y+1)]*a[7]
                    +originalImage->data[x+1+originalImage->width*(y+1)]*a[8];
      double dfdj = originalImage->data[x+originalImage->width*(y-1)]*b[1]
                    +originalImage->data[x+1+originalImage->width*(y-1)]*b[2]
                    +originalImage->data[x+originalImage->width*y]*b[4]
                    +originalImage->data[x+1+originalImage->width*y]*b[5]
                    +originalImage->data[x+originalImage->width*(y+1)]*b[7]
                    +originalImage->data[x+1+originalImage->width*(y+1)]*b[8];
      double result = sqrt(pow(dfdi,2)+pow(dfdj,2));
      result = valuecheck(result);
      resultImage->data[resultImage->width*y]=(unsigned char)result;                      
    }
    resultImage->data[0]=originalImage->data[0];/*四隅は元の画像の階級値をそのまま使う*/
    resultImage->data[resultImage->width-1]=originalImage->data[originalImage->width-1];
    resultImage->data[resultImage->width*(resultImage->height-1)]=originalImage->data[originalImage->width*(originalImage->height-1)];
    resultImage->data[resultImage->width*resultImage->height-1]=originalImage->data[originalImage->width*originalImage->height-1];
}

/*======================================================================
 * PGM-RAW フォーマットのヘッダ部分の書き込み
 *======================================================================
 *   画像構造体 image_t *ptImage の内容に従って、出力ファイル FILE *fp
 * に、PGM-RAW フォーマットのヘッダ部分を書き込む。
 */
void
writePgmRawHeader(FILE *fp, image_t *ptImage)
{
    /* マジックナンバー(P5) の書き込み */
    if(fputs("P5\n", fp)==EOF)
    {
        goto error;
    }

    /* 画像サイズの書き込み */
    if (fprintf(fp, "%d %d\n", ptImage->width, ptImage->height)==EOF)
    {
        goto error;
    }

    /* 画素値の最大値を書き込む */
    if (fprintf(fp, "%d\n", ptImage->maxValue)==EOF)
    {
        goto error;
    }

    return;

error:
    fputs("Writing PGM-RAW header was failed\n", stderr);
    exit(1);
}


/*======================================================================
 * PGM-RAWフォーマットの画素値データの書き込み
 *======================================================================
 *   画像構造体 image_t *ptImage の内容に従って、出力ファイル FILE *fp
 * に、PGM-RAW フォーマットの画素値データを書き込む
 */
void
writePgmRawBitmapData(FILE *fp, image_t *ptImage)
{
    if( fwrite(ptImage->data, sizeof(unsigned char),
            ptImage->width * ptImage->height, fp)
            != ptImage->width * ptImage->height )
    {
        /* エラー */
        fputs("Writing PGM-RAW bitmap data was failed\n", stderr);
        exit(1);
    }
}
 

/*
 * メイン
 */
int
main(int argc, char **argv)
{
    image_t originalImage, resultImage;
    FILE *infp, *outfp;
  
    /* 引数の解析 */
    parseArg(argc, argv, &infp, &outfp);

    /* 元画像の画像ファイルのヘッダ部分を読み込み、画像構造体を初期化 */
    /* する */
    readPgmRawHeader(infp, &originalImage);

    /* 元画像の画像ファイルのビットマップデータを読み込む */
    readPgmRawBitmapData(infp, &originalImage);

    /* 結果画像の画像構造体を初期化する。画素数、階調数は元画像と同じ */
    initImage(&resultImage, originalImage.width, originalImage.height,
            originalImage.maxValue);

    /* フィルタリング */
    filteringImage(&resultImage, &originalImage);

    /* 画像ファイルのヘッダ部分の書き込み */
    writePgmRawHeader(outfp, &resultImage);

    /* 画像ファイルのビットマップデータの書き込み */
    writePgmRawBitmapData(outfp, &resultImage);

    return 0;
}
