#pragma once
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

// for memcpy
#include <cstring>
#include <cstdlib>
#include <stdint.h>
#include <climits>

// our time struct
#include <ctime>
#include <sys/timeb.h>

// c++ part
#include <string>
#include <map>
#include <functional>
#include <unordered_map>

// var args
#include <cstdarg>

#define println(msg) printf("%s\n", msg)
#define printd(n){printf("%d\n", n);}
#define printld(n){printf("%ld\n", n);}
#define printcd(cond){printf("condition is : %s\n", cond? "true" : "false");}
#define printst(str){printf("strlen : %ld\n", strlen(str));}
#define array_length(arr)({int ret = sizeof(arr)/sizeof(*arr); ret;})
#define FORMAT_BUFFER 1024

#define copy_struct(a)({\
  size_t size = sizeof(a);\
  void* b = malloc(size);\
  memset(b, 0, size);\
  memccpy(b, a, 1, size);  \
  b;\
})

bool cmp_str(const char* ch1, const char* ch2){
  size_t len = strlen(ch2);
  if(strlen(ch1)!= len) return false;
  for(size_t i=0; i<len; i++){
    if(ch1[i]!=ch2[i]) return false;
  }
  return true;
}

using namespace std;

typedef struct record{
  int id;
  long created;
  long datetime;
  int user_id;
  char session_name [100];
  double latitude;
  double longitude;
  char ip_address [50];
  char online_status [15];
  char device_id [65];
  char device_model [50];
  char os_version [20];
  int client_id;
  char client_version [20];
  int item_id;
  char page_orientation [20];
  char page_number [50];
  double second;
  int organization_id;
  int catalog_id;
}record;
static size_t recordsize = sizeof(struct record);




// LIST STRUCTURES
template <typename T>
struct node{
  T data;
  node<T>* next;
};

template <typename T>
class GList{
  protected:
    int _size;
    size_t struct_size;
    node<T>* head;
    node<T>* curr;
  public: 
    GList(){
      struct_size = sizeof(struct node<T>);
      head = NULL;
      curr = NULL;
      _size = 0;
    }
    virtual ~GList(){
      while ((curr = head) != NULL)
      {                      // set curr to head, stop if list empty.
          head = head->next; // advance head to next element.
          free_data(curr->data);
          free(curr);        // delete saved pointer.
      }
    };
    void add(T _data){
      if(!_size){
        head = (node<T>*)malloc(struct_size);
        head->data = _data;
        curr = head;
      } else {
        curr->next = (node<T>*)malloc(struct_size);
        curr = curr->next;
        curr->data = _data;
      }
      _size++;
    }
    node<T>* iterator(){
      node<T>* i = (node<T>*)copy_struct(head);
      i->next = head->next;
      return i;
    }
    // method hook to perform extra cleanup
    virtual void free_data(T t){};
    int size(){
      return _size;
    }
};

class RecordList : public GList<record *>
{
  private:
    double _reading_time;
  public:
    RecordList(){};
    //~RecordList(){};
    void copy(record *r)
    {
        GList<record *>::add((record *)copy_struct(r));
        _reading_time += r->second;
    }
    void free_data(record *r)
    {
        free(r);
    }
    double reading_time(){
        return _reading_time;
    }
};
// END LIST STRUCTURES


template <typename K>
class RecordMap : public unordered_map<K, RecordList*>{
public:
  void add_copy(K key, record* rec){
    typename RecordMap<K>::iterator it = this->find(key);
    if(it != this->end()){ // has previous entry
        it->second->copy(rec);
    } else { // no previous entry
        RecordList* _list = new RecordList;
        _list->copy(rec);
        this->insert(pair<K, RecordList*>(key, _list));
    }
  }

  void add(K key, record* rec){
    typename RecordMap<K>::iterator it = this->find(key);
    if(it != this->end()){ // has previous entry
        it->second->add(rec);
    } else { // no previous entry
        RecordList* _list = new RecordList;
        _list->add(rec);
        this->insert(pair<K, RecordList*>(key, _list));
    }
  }
};


class CompositeMap : public unordered_map<string, RecordList*>{
public:
  void add_copy(string key, record* rec){
    unordered_map<string, RecordList*>::iterator it = this->find(key);
    if(it != this->end()){ // has previous entry
        it->second->copy(rec);
    } else { // no previous entry
      RecordList* _list = new RecordList;
      _list->copy(rec);
      this->insert(pair<string, RecordList*>(string(key.c_str()), _list));
    }
  }

  void add(string key, record* rec){
    unordered_map<string, RecordList*>::iterator it = this->find(key);
    if(it != this->end()){ // has previous entry
        it->second->add(rec);
    } else { // no previous entry
      RecordList* _list = new RecordList;
      _list->add(rec);
      this->insert(pair<string, RecordList*>(string(key.c_str()), _list));
    }
  }
};

class Summarizer{
  private:
    int _count;
    double _read_time;
    record rec;
  public:
    Summarizer(record* _rec){
      memcpy(&rec, _rec, sizeof(record)); // copy data to represent aggregate group
    };
    virtual ~Summarizer(){};
    void sum(record* r){
      _count++;
      _read_time += r->second;
    };

    record* get_record(){
      return &rec;
    }

    int count(){return _count;}
    double reading_time(){return _read_time;}
};

template<typename T>
class RecordSummarizer : public unordered_map<T, Summarizer*>{
  public:
    void add(T key, record* rec){
      typename RecordSummarizer<T>::iterator it = this->find(key);
      if(it != this->end()){ // has previous entry
          it->second->sum(rec);
      } else { // no previous entry
          Summarizer* s = new Summarizer(rec);
          s->sum(rec);
          this->insert(pair<T, Summarizer*>(key, s));
      }
    }
};

class CompositeSummarizer : public unordered_map<string, Summarizer*>{
  public:
    void add(string key, record* rec){
      unordered_map<string, Summarizer*>::iterator it = this->find(key);
      if(it != this->end()){ // has previous entry
          it->second->sum(rec);
      } else { // no previous entry
          Summarizer* s = new Summarizer(rec);
          s->sum(rec);
          this->insert(pair<string, Summarizer*>(string(key.c_str()), s));
      }
    }
};

#define cd_id(r,_id){{r->id == _id;}}
#define cd_created(r,_created)({r->created == _created;})
#define cd_datetime(r,_datetime)({r->datetime == _datetime;})
#define cd_user_id(r,_user_id)({r->user_id == _user_id;})
#define cd_session_name(r,_session_name)({cmp_str(r->session_name, _session_name);})
#define cd_lattitude(r,_lattitude)({r->lattitude == _lattitude;})
#define cd_longitude(r,_longitude)({r->longitude == _longitude;})
#define cd_ip_address(r,_ip_address)({cmp_str(r->ip_address, _ip_address);})
#define cd_online_status(r,_online_status)({cmp_str(r->online_status, _online_status);})
#define cd_device_id(r,_device_id)({cmp_str(r->device_id, _device_id);})
#define cd_device_model(r,_device_model)({cmp_str(r->device_model, _device_model);})
#define cd_os_version(r,_os_version)({cmp_str(r->os_version, _os_version);})
#define cd_client_id(r,_client_id)({r->client_id == _client_id;})
#define cd_client_version(r,_client_version)({cmp_str(r->client_version, _client_version);})
#define cd_item_id(r,_item_id)({r->item_id == _item_id;})
#define cd_page_orientation(r,_page_orientation)({cmp_str(r->page_orientation, _page_orientation);})
#define cd_page_number(r,_page_number)({cmp_str(r->page_number, _page_number);})
#define cd_second(r,_second)({r->second == _second;})(
#define cd_organization_id(r,_organization_id)({r->organization_id == _organization_id;})
#define cd_catalog_id(r,_catalog_id)({r->catalog_id == _catalog_id;})


// format output char using fixed size buffer, and copy it to destination target
// don't forget to put that null terminator so we can compare it's strlen
static char f_buf [FORMAT_BUFFER];
static void str_format(char* format, int args, char* dest){
  int len = snprintf(f_buf, FORMAT_BUFFER, format, args);
  memcpy(dest, f_buf, len);
  dest[len] = '\0';
}

static void str_copy(const char* src, char* dest){
  int len = strlen(src);
  memset(dest, 0, len);// clean the garbage first
  memcpy(dest, src, len);
  dest[len] = '\0'; // add null terminator
}

// get file size using sys/stat.h
size_t getFilesize(const char* filename) {
    struct stat st;
    stat(filename, &st);
    return st.st_size;
}

static int getMilliCount(){
	timeb tb;
	ftime(&tb);
	int nCount = tb.millitm + (tb.time & 0xfffff) * 1000;
	return nCount;
}

static int getMilliSpan(int nTimeStart){
	int nSpan = getMilliCount() - nTimeStart;
	if(nSpan < 0)
		nSpan += 0x100000 * 1000;
	return nSpan;
}

static void print_record(record* r){
  printf("{id: %d, created: %ld, datetime: %ld, user_id: %d, session_name: '%s', lattitude: %f,\
  longitude: %f, ip_address: '%s', online_status:'%s', device_id: '%s', device_model: '%s', os_version:'%s',\
  client_id: %d, client_version: '%s', item_id: %d, page_orientation: '%s', page_number: '%s', \
  second: '%f', organization_id: %d, catalog_id: %d}\n",\
  r->id, r->created, r->datetime, r->user_id, r->session_name, r->latitude, r->longitude, r->ip_address,\
  r->online_status, r->device_id, r->device_model, r->os_version, r->client_id, r->client_version,\
  r->item_id, r->page_orientation, r->page_number, r->second, r->organization_id, r->catalog_id);
}

static inline size_t write_record(FILE* file, record* rec) {
  return fwrite(rec, recordsize, 1, file);
}

static inline size_t read_record(FILE* file, record* rec){
  return fread(rec, recordsize, 1, file);
}