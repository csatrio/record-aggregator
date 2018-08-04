#include "records.h"
#include <iostream>
#include <sstream>
using namespace std;

static void iterate_records(int loop, const char* filename){
  FILE* file = fopen(filename, "rb");
  record* r = (record*)malloc(recordsize);
  int start = getMilliCount();
  CompositeSummarizer map = CompositeSummarizer();
  ostringstream ss;

  for(int i=0;i<loop; i++){
    read_record(file, r);
    ss.str("");
    ss.clear();
    ss << r->item_id;
    //ss << r->user_id;
    //ss << r->device_model;
    map.add(ss.str(), r);
  }
  fclose(file);

  int j = 0;
  double read_time = 0;
  for(CompositeSummarizer::iterator it=map.begin(); it!=map.end(); it++){
    Summarizer* rlist = it->second;
    record* rec = rlist->get_record();
    read_time += rlist->reading_time();
    printf("item_id : %d\n", rec->item_id);
    //printf("item_id : %d, user_id: %d, reading_time:%f \n", r->item_id, 0, rlist->reading_time());
    j++;
  }
  printf("Total records: %d\n", loop);
  printf("Total records mapped : %d\n", j);
  printf("Mapping ratio percentage : %f\n", ((float)j/(float)loop)*(float)100);
  printf("Total reading time : %f\n", read_time);

  print_record(r);
  printf("Time taken to iterate all records : %d ms", getMilliSpan(start));
  free(r);
}

int main(int argc, char* argv[]) {
  const char* filename = "/home/csatrio/Desktop/records.mmap";
  int iteration = 5897025;
  //int iteration = 50000;
  iterate_records(iteration, filename);
  return 0;
}