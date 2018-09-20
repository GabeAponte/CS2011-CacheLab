//Hunter Trautz and Gabriel Aponte
//Team Name: hctautz-gaaponte
#include <stdlib.h>
#include "cachelab.h"
#include <getopt.h>
#include <stdio.h>
#include <math.h>

//64 bit variable type we will use to hold memory addresses
typedef unsigned long long int memAddress;

//struct used to store and group all the needed paramters of the cache together
struct Parameters {
  int s; //number of set index bits (the number of sets is 2^s)
  int S; //total number of sets (calculated from 2^s)
  int E; //number of lines per set (associativity)
  int b; //number of block bits (the block size is 2^b)
  int B; //block size (calculated from 2^b)
  int hits; //Number of hits
  int misses; //Number of misses
  int evicts; //Number of evictions
  int verbose; //verbosity flag
};

//lines that will be stored in sets
struct setLine{
  int uses;
  int valid;
  unsigned long long tag;
  char *block;
};

//a set is made up of lines
struct cacheSet{
  struct setLine *lines;
};

//a cache is made up of of sets which contain lines
struct Cache{
  struct cacheSet *sets;
};

struct Cache build(long long totalSets, int totalLines, int blockSize){
  struct Cache newCache;
  struct cacheSet set;
  struct setLine line;

  //allocate memory for the new cache based off of how many total sets it will have
  newCache.sets = (struct cacheSet*) malloc(sizeof(struct cacheSet)* totalSets);

  for (int i = 0; i < totalSets; i++){
    //alocate memory for each line in each set of the new cache
    set.lines = (struct setLine*) malloc (sizeof(struct setLine) * totalLines);
    //assigns the newly created sets to the cache
    newCache.sets[i] = set;

    //sets the value of every field in each line to 0 and then assigns them to the sets withtin the cache
    for (int j = 0; j < totalLines; j ++){
      line.uses = 0;
      line.valid = 0;
      line.tag = 0;
      set.lines[j] = line;
    }
  }
  return newCache;
}

void printUsage(){
    printf("Usafe: /csim-ref [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n");
    printf("-h: Optional help flag that prints usage info\n");
    printf("-v: Optional verbose flag that displays trace info\n");
    printf("-s <s>: Number of set index bits (the number of sets is 2^s)\n");
    printf("-E <E>: Associativity (number of lines per set)\n");
    printf("-b <b>: Number of block bits (the block size is 2^b)\n");
    printf("-t <tracefile>: Name of the valgrind trace to replay\n");
}

//returns the line number of the least recently used line (meaning that it meets the requirments for eviction)
int findLRU(struct cacheSet set, struct Parameters param){
  int totalLines = param.E;
  int minUses = set.lines[0].uses;
  int LRU = 0;

  //loop through every line in the set, saving the line number of the line with the least uses
  //and its number of uses for comparison with the rest of the set, replacing it if another line
  //is found with less uses
  for(int i = 0; i < totalLines; i++){
    if(set.lines[i].uses < minUses){
      LRU = i;
      minUses = set.lines[i].uses;
    }
  }
  return LRU;
}

//returns the line number of the most recently used line (meaning that it belongs in the cache)
int findMRU(struct cacheSet set, struct Parameters param){
  int totalLines = param.E;
  int maxUses = set.lines[0].uses;
  int MRU = 0;

  //loop through every line in the set, saving the line number of the line with the most uses
  //and its number of uses for comparison with the rest of the set, replacing it if another line
  //is found with more uses
  for(int i = 0; i < totalLines; i++){
    if(set.lines[i].uses > maxUses){
      MRU = i;
      maxUses = set.lines[i].uses;
    }
  }
  return MRU;
}

//Determines if there is a hit
int checkHit(struct setLine line, memAddress address){
  return line.tag == address && line.valid;
}

//Determines if a given set is full
int isSetFull(struct cacheSet set, struct Parameters param){
  int totalLines = param.E;
    for(int i=0;i<totalLines;i++){
      //if we find an empty line then the set is not full, so return 1
        if(set.lines[i].valid == 0){
            return 1;
        }
    }
    //no empty lines so the set is full
    return 0;
}

//checks whether any lines in a given set are empty and returns the number of
//the empty line if one exists, if there are none then return 0 because therefore the set is full
int anyEmptyLines(struct cacheSet set, struct Parameters param){
  int totalLines = param.E;
  struct setLine line;

  for (int i = 0; i < totalLines; i++){
    line = set.lines[i];
    //if a line is not valid return its line number
    if(line.valid == 0){
      return i;
    }
  }
  return 0;
}

struct Parameters Simulation(struct Cache aCache, struct Parameters param, memAddress address){
  //calculate the size of the tag by subtratcing the
  //sum of the number of block bits and the number of sets in the cache from 64
  int tagSize = 64-(param.b + param.s);
  //calculate the address tag by shifting the address to the right by the sum of the sets and block bits
  memAddress addressTag = address >> (param.s + param.b);
  //calculate the index number of the set within the cache
  unsigned long long setNum = (address << (tagSize)) >> (tagSize + param.b);

  struct cacheSet set = aCache.sets[setNum];
  //will keep track of any hits that happened as we loop through the set
  int hit = 0;
  for (int i = 0; i < param.E; i++){
    struct setLine line = set.lines[i];
    //there was a hit, increase the hit and usescounter,also change the flag
    if(checkHit(line, addressTag) == 1){
      //increment hits
      param.hits++;
      hit = 1;
      //increment uses
      aCache.sets[setNum].lines[i].uses = aCache.sets[setNum].lines[findMRU(set, param)].uses+1;
      break;
    }
  }

  //if there was a miss and the set is not full than place the miss in the nearest empty line
  if(hit == 0 && isSetFull(set, param) == 1){
    param.misses++;
    //find the next empty line for the miss
    int emptyLine = anyEmptyLines(set, param);
    //set the address tag
    set.lines[emptyLine].tag = addressTag;
    //mark the line as valid as it now contains something
    set.lines[emptyLine].valid = 1;

    int MRU = findMRU(set, param);
    //increment uses
    aCache.sets[setNum].lines[emptyLine].uses = aCache.sets[setNum].lines[MRU].uses+1;
  }
  /* otherwise we have to evict */
   else if(hit ==0){
    param.misses++; //increment misses
    param.evicts++; //increment evictions

    //we need to evict the LRU to make space for the miss
    int LRU = findLRU(set, param);
    //replace the lines tag
    set.lines[LRU].tag = addressTag;

    int mostRecentlyUsed = findMRU(set, param);
    //increment uses
    aCache.sets[setNum].lines[LRU].uses = aCache.sets[setNum].lines[mostRecentlyUsed].uses+1;
  }
  return param;
}

//loops through every line and set in the cache and free every set and line that isn't null
void clearCache(struct Cache aCache, long long totalSets, int totalLines, long long blockSize){
  for (int i = 0; i < totalSets; i++){
    struct cacheSet set = aCache.sets[i];

    if(set.lines != NULL){
      free(set.lines);
    }
  }

  if(aCache.sets != NULL){
    free(aCache.sets);
  }
}

int main(int argc, char **argv){
  struct Cache simulatedCache;
	struct Parameters param;
	long long totalSets;
	long long blockSize;

	char *traceFile;
	char c;

    while( (c=getopt(argc,argv,"s:E:b:t:vh")) != -1){
        switch(c) {
        case 's':
            param.s = atoi(optarg);
            break;
        case 'E':
            param.E = atoi(optarg);
            break;
        case 'b':
            param.b = atoi(optarg);
            break;
        case 't':
            traceFile = optarg;
            break;
        case 'v':
            param.verbose = 1;
            break;
        case 'h':
            printUsage();
            exit(0);
        default:
            printUsage();
            exit(0);
        }
    }

 	totalSets = pow(2.0, param.s);   // get Number of set by 2^s
	blockSize = pow(2.0, param.b);  // get blockSize by 2^b

	param.hits = 0;
	param.misses = 0;
	param.evicts = 0;

	simulatedCache = build(totalSets, param.E, blockSize);

  FILE *openTrace;
  char instruction;
  memAddress address;
  int size;

	openTrace = fopen(traceFile, "r");
	if (openTrace != NULL) {
		while (fscanf(openTrace, " %c %llx,%d", &instruction, &address, &size) == 3) {
			switch(instruction) {
				case 'I':
					break;
				case 'L':
					param = Simulation(simulatedCache, param, address);
					break;
				case 'S':
					param = Simulation(simulatedCache, param, address);
					break;
				case 'M':
					param = Simulation(simulatedCache, param, address);
					param = Simulation(simulatedCache, param, address);
					break;
				default:
					break;
			}
		}
	}
  printSummary(param.hits, param.misses, param.evicts);
	clearCache(simulatedCache, totalSets, param.E, blockSize);
	fclose(openTrace);
  return 0;
}
