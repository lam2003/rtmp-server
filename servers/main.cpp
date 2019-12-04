#include <core.hpp>
#include <repo_version.h>

int main(int argc,char *argv[]){
    printf("REPO_VERSION:%s\n",REPO_VERSION);
    printf("REPO_DATE:%s\n",REPO_DATE);
    printf("REPO_HASH:%s\n",REPO_HASH);
}