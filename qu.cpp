// qu.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

static unsigned char mask[8] =
{ 0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01 };
unsigned int maxcolor = 16;
unsigned int currentcolor = 0;
bool pri = false;

struct node
{
   bool IsLeaf;
   bool Reduce;
   unsigned int count;
   unsigned int level;//当 level = 7就是叶子节点

   unsigned int redsum;
   unsigned int greensum;
   unsigned int bluesum;
   unsigned int mapto;
   unsigned int childnum;
   node* ptrChild[8];
   node(unsigned int lev)
   {
       level = lev;
       redsum = greensum = bluesum =  count = childnum =  0;
       mapto = -1;
       for (int i = 0; i < 8; i++)
       {
           ptrChild[i] = NULL;
       }
       IsLeaf =  false;
       Reduce = true;
   }
   
};
std::vector<node*> LeafNodes;
std::vector<int> HeadNodes[8];

void RecursiveReduce(int idx)
{
    if (LeafNodes[idx]->Reduce == false)
    {
        return;
    }
    LeafNodes[idx]->Reduce = false;
    for (int i = 0; i < 8; i++)
    {
        if (LeafNodes[idx]->ptrChild[i] != NULL)
            RecursiveReduce(LeafNodes[idx]->ptrChild[i]->mapto);
    }
}

void ReduceColor()
{
    int minidx = -1;
    for (int i = 7; i >=0 ; i--)
    {
        int mincount = 0x7f7f7f;
        for (int j = 0; j < HeadNodes[i].size(); j++)
        {
            int idx = HeadNodes[i][j];
            if (LeafNodes[idx]->Reduce && LeafNodes[idx]->childnum >= 2 && mincount > LeafNodes[idx]->childnum)
            {
                mincount = LeafNodes[idx]->childnum;
                minidx = idx;
            }
        }
        if (minidx != -1)
            break;
    }
    RecursiveReduce(minidx);
    LeafNodes[minidx]->IsLeaf = true;
    currentcolor -= (LeafNodes[minidx]->childnum - 1);
    if (pri)printf("得到索引%d,目前颜色数量%d！\n", minidx,currentcolor);
}

void addColor(node* &anode,unsigned int red, unsigned int green, unsigned int blue,bool newnode)
{  
    if (anode->level < 8 && newnode)//将该节点的索引放进所在层中，方便之后搜索
    {
        HeadNodes[anode->level].push_back(anode->mapto);
    }
    anode->redsum += red;
    anode->greensum += green;
    anode->bluesum += blue;
    anode->count += 1;
    if (anode->IsLeaf)
        return;

    if (anode->level == 8)//如果是第8层
    {
        anode->IsLeaf = true;
        anode->Reduce = false;
        if (newnode)//新颜色，需要新建一个叶子节点
        {
            currentcolor += 1;
            anode->IsLeaf = true;
            if (currentcolor > maxcolor)
            {
                if (pri)printf("需要减去一个颜色！\n");
                ReduceColor();
            }
         }
    }
    else
    {
        unsigned int shift = 7 - anode->level;
        unsigned int Idx = (((red & mask[anode->level]) >> shift) << 2) |
            (((green & mask[anode->level]) >> shift) << 1) |
          ((blue & mask[anode->level]) >> shift);
        if (anode->ptrChild[Idx] == nullptr)//子节点不存在，需要新建一个
        {
           
            anode->ptrChild[Idx] = new node(anode->level + 1);
            LeafNodes.push_back(anode->ptrChild[Idx]);
            anode->ptrChild[Idx]->mapto = LeafNodes.size() - 1;
            anode->childnum += 1;
            addColor(anode->ptrChild[Idx], red, green, blue,true);
        }
        else
        {
            addColor(anode->ptrChild[Idx], red, green, blue, false);
        }
    }
}

int QueryColor(node*& anode, unsigned int red, unsigned int green, unsigned int blue)
{
    if (anode->IsLeaf)
    {
        return anode->mapto;
    }
    else
    {
        unsigned int shift = 7 - anode->level;
        unsigned int Idx = (((red & mask[anode->level]) >> shift) << 2) |
            (((green & mask[anode->level]) >> shift) << 1) |
            ((blue & mask[anode->level]) >> shift);
        if (pri) printf("shift = %d,idx = %d，下一层数是%d\n", shift, Idx, anode->level + 1);
        return QueryColor(anode->ptrChild[Idx], red, green, blue);
    }
}

int main()
{
    int width, height, channels;
    unsigned char* img = stbi_load("bread.png", &width, &height, &channels, 0);
    if (img == NULL) {
            printf("Error in loading the image\n");
              exit(1);
    }

    node* RootNode = new node(0);
    LeafNodes.push_back(RootNode);
    RootNode->mapto = 0;
    HeadNodes[0].push_back(0);
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            int idx = (i * width + j)*channels;

          //   printf("idx = %d,R = %d,G = %d,B = %d\n", idx, img[idx], img[idx + 1], img[idx + 2]);
           addColor(RootNode, img[idx], img[idx + 1], img[idx + 2],false);

        }
    }
    printf("ova\n");
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            int idx = (i * width + j) * channels;
            int vectoridx =  QueryColor(RootNode, img[idx], img[idx + 1], img[idx + 2]);
            if (pri)  printf("vectoridx = %d\n", vectoridx);
            img[idx] = LeafNodes[vectoridx]->redsum / LeafNodes[vectoridx]->count;
           img[idx + 1] = LeafNodes[vectoridx]->greensum / LeafNodes[vectoridx]->count;
            img[idx + 2] = LeafNodes[vectoridx]->bluesum / LeafNodes[vectoridx]->count;

        }
    }

    stbi_write_png("sky.png", width, height, channels, img, width * channels);
    stbi_image_free(img);
}