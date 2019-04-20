/**
 * Replaces rank in the pipe described in CSC417
 * Uses the pipe/filter pattern: 
 * getLine | toColumns | populate | bestRest | countRanges | filter | assignScores | sort | print
 * @file rank.c
 * @author Spencer Yoder
 * @author Thea Wall
 * @author Josef Dewberry
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

/** The initial storage for the dynamically resizable column list */
#define INIT_COL_STORAGE 100
/** The width of the buffer for a column cell */
#define COL_WIDTH_BUFFER 50
/** The width of the buffer for a line */
#define LINE_BUFFER 200

/** The number of columns */
int numColumns = 0;
/** The symbol in !klass column for best */
char bestVal[COL_WIDTH_BUFFER];
/** The symbol in !klass column for rest */
char restVal[COL_WIDTH_BUFFER];


/** Stores the name of a column and a dynamically resizable array of either numbers or strings */
typedef struct ColumnStruct {
  /** The column title */
  char name[COL_WIDTH_BUFFER];
  /** The number of cells in the column */
  int size;
  /** The length of the dynamically resizable array */
  int length;
  /** A dynamically resizable array of cell values */
  char **strings;
  /** Whether or not this column is independent */
  bool independent;
  /** The total number of unique values */
  int uniques;
} Column;

/** A range within a column */
typedef struct RangeStruct {
  /** The name of the column for this range */
  char colName[COL_WIDTH_BUFFER];
  /** The value for this range */
  char value[COL_WIDTH_BUFFER];
  /** The weighted number of appearances in best */
  double bestCount;
  /** The weighted number of appearances in rest */
  double restCount;
  /** The score for this range */
  double score;
} Range;

/**
 * Removes whitespace at the front and back of src and copies that result to dest
 * @param src the string to be trimmed and copied
 * @param dest the destination for the copied string
 */
void trimCopy(char *dest, char *src) {
  while(isspace(*src))
    src++;
  
  int len = strlen(src);
  for(int i = len - 1; i >= 0; i--) {
    if(isspace(src[i])) {
      src[i] = '\0';
    } else {
      strcpy(dest, src);
      return;
    }
  }
}

/**
 * Constructs a new column with the given name
 * @param name the name of the column
 * @return a pointer to the constructed column
 */
Column *newColumn(char name[COL_WIDTH_BUFFER]) {
  Column *c = malloc(sizeof(Column));
  trimCopy(c->name, name);
  c->size = 0;
  c->length = INIT_COL_STORAGE;
  c->strings = malloc(sizeof(char *) * INIT_COL_STORAGE);
  char first = name[0];
  c->independent = first == '$' || (first != '<' && first != '>' && first != '!');
  c->uniques = 0;
  return c;
}

/**
 * Destroy the given array of column pointers
 * @param columns the array of column pointers
 */
void destroyColumns(Column **columns) {
  for(int i = 0; i < numColumns; i++) {
    Column *c = columns[i];
    for(int j = 0; j < c->size; j++) {
      if(c != NULL)
        free(c->strings[j]);
    }
    free(c->strings);
    free(c);
  }
  free(columns);
}

/**
 * Adds a string to the given column
 * @param c the given column
 * @param value the string to add
 */
void addString(Column *c, char *value) {
  //If we run out of room in the buffer
  if(c->size >= c->length) {
    //Make some more and copy everything
    char **copy = malloc(sizeof(char *) * c->length * 2);
    for(int i = 0; i < c->length; i++) {
      copy[i] = malloc(sizeof(char) * COL_WIDTH_BUFFER);
      strcpy(copy[i], c->strings[i]);
      free(c->strings[i]);
    }
    free(c->strings);
    c->strings = copy;
    c->length *= 2;
  }
  c->strings[c->size] = malloc(sizeof(char) * COL_WIDTH_BUFFER);
  strcpy(c->strings[c->size++], value);
}

/**
 * @return a new range with the given name and value
 */
Range *newRange(char colName[COL_WIDTH_BUFFER], char value[COL_WIDTH_BUFFER]) {
  Range *r = malloc(sizeof(Range));
  strcpy(r->colName, colName);
  strcpy(r->value, value);
  r->bestCount = 0;
  r->restCount = 0;
  return r;
}

/**
 * Adds a value to the given list of ranges.
 * This only stores unique ranges, if a value is not unique, the corresponding struct has its count
 * incremented.
 * @param ranges the list of ranges
 * @param colName the name for the column this range is in
 * @param value the value for the range
 * @param best whether or not this value appears in best or rest
 * @param count the size of the best/rest list
 */
void addValue(Range **ranges, char colName[COL_WIDTH_BUFFER], char value[COL_WIDTH_BUFFER], 
              bool best, int count) {
  Range *range = ranges[0];
  int idx = 0;
  while(range != NULL) {
    //If we already have a range with the given value
    if(strcmp(range->value, value) == 0) {
      //Increment the score accordingly
      if(best) {
        range->bestCount += 1 / (double) count;
      } else {
        range->restCount += 1 / (double) count;
      }
      return;
    }
    range = ranges[++idx];
  }
  //Otherwise, add a new range with the correct score
  ranges[idx] = newRange(colName, value);
  if(best) {
    ranges[idx]->bestCount = 1 / (double) count;
  } else {
    ranges[idx]->restCount = 1 / (double) count;
  }
}

/**
 * @return a line from standard input
 */
char *getLine() {
  char *buffer = malloc(sizeof(char) * LINE_BUFFER);
  int c = getchar();
  if(c == EOF) {
    return NULL;
  }
  int i;
  for(i = 0; i < LINE_BUFFER - 1; i++) {
    if(c == EOF || c == '\n') {
      break;
    }
    buffer[i] = (char) c;
    c = getchar();
  }
  buffer[i] = '\0';
  return buffer;
}

/**
 * Reads the first line of input containing the column names and returns an array of column pointers
 * @param names the line of input
 * @return the array of column pointers
 */
Column **toColumns(char *names) {
  int idx = 0;
  int nameIdx = 0;
  char c = names[idx++];
  char name[COL_WIDTH_BUFFER];
  //We have room for 50 columns, if there are more than 50, we're out of luck
  Column **columns = malloc(sizeof(Column *) * COL_WIDTH_BUFFER);
  //This for loop is responsible for parsing a csv line for individual words
  for(;;) {
    //If we're at the end of the line, add the built string to the columns list
    if(c == '\0') {
      name[nameIdx] = '\0';
      columns[numColumns++] = newColumn(name);
      break;
    }
    //If we reach a comma, we can cap off the string we're building
    if(c == ',') {
      name[nameIdx] = '\0';
      columns[numColumns++] = newColumn(name);
      nameIdx = 0;
      //Check for debugging
      if(numColumns >= COL_WIDTH_BUFFER) {
        printf("ERROR: NUM COLUMNS EXCEDING BUFFER SIZE\n");
        exit(1);
      }
    //Otherwise, build on to the current string
    } else {
      name[nameIdx++] = c;
    }
    //Get the next character from the line
    c = names[idx++];
  }
  //Make only enough space for the number of columns that there actually are
  Column **final = malloc(sizeof(Column *) * numColumns);
  for(int i = 0; i < numColumns; i++) {
    final[i] = columns[i];
  }
  //We don't need the too-big column list anymore
  free(columns);
  //We don't need the line anymore
  free(names);
  return final;
}

/**
 * Populate the given array of columns with cells parsed from standard input
 * @param columns the array of columns
 * @return the populated array
 */
Column **populate(Column **columns) {
  char *line = getLine();
  while(line != NULL) {
    int lineIdx = 0;
    char c = line[lineIdx++];
    int colIdx = 0;
    char cell[COL_WIDTH_BUFFER];
    int cellIdx = 0;
    //We are again parsing the csv line for values, see above for details
    for(;;) {
      if(c == '\0') {
        cell[cellIdx] = '\0';
        addString(columns[colIdx], cell);
        break;
      }
      if(c == ',') {
        cell[cellIdx] = '\0';
        addString(columns[colIdx], cell);
        colIdx++;
        cellIdx = 0;
      } else {
        cell[cellIdx++] = c;
      }
      c = line[lineIdx++];
    }
    line = getLine();
  }
  return columns;
}

/**
 * Divide the given array into two new arrays: best - which has the higher of the two klass scores
 * and rest - which has the lower of the two klass scores
 * @param columns the array of columns
 * @return two arrays of columns, best and rest
 */
Column ***bestRest(Column **columns) {
  //The last column only has two values. One is higher, this is best, the other value is rest
  Column *klass = columns[numColumns - 1];
  char oneVal[COL_WIDTH_BUFFER];
  strcpy(oneVal, klass->strings[0]);
  for(int i = 1; i < klass->size; i++) {
    if(strcmp(oneVal, klass->strings[i]) != 0) {
      //The lowest value will always start with '..'
      if(oneVal[0] == '.') {
        strcpy(restVal, oneVal);
        strcpy(bestVal, klass->strings[i]);
      } else {
        strcpy(restVal, klass->strings[i]);
        strcpy(bestVal, oneVal);
      }
      break;
    }
  }
  
  Column **best = malloc(sizeof(Column *) * numColumns);
  Column **rest = malloc(sizeof(Column *) * numColumns);
  //Best and rest both have their own columns
  for(int i = 0; i < numColumns; i++) {
    best[i] = newColumn(columns[i]->name);
    rest[i] = newColumn(columns[i]->name);
  }
  
  //For each row, if the row has the best value, add it to best, otherwise, add to rest
  for(int i = 0; i < klass->size; i++) {
    if(strcmp(klass->strings[i], bestVal) == 0) {
      for(int j = 0; j < numColumns; j++) {
        addString(best[j], columns[j]->strings[i]);
      }
    } else {
      for(int j = 0; j < numColumns; j++) {
        addString(rest[j], columns[j]->strings[i]);
      }
    }
  }
  
  //We don't need the original column list anymore
  destroyColumns(columns);
  Column ***bestRest = malloc(sizeof(Column **) * 2);
  bestRest[0] = best;
  bestRest[1] = rest;
  return bestRest;
}

/**
 * Create a seperate Range struct for each unique range in best and rest
 * @param rowGroups the rows in best and rest
 * @return a list of unique ranges for each column
 */
Range ***countRanges(Column ***rowGroups) {
  //The outer array is for each column
  Range ***rangeGroups = malloc(sizeof(Range **) * numColumns);
  for(int i = 0; i < numColumns; i++) {
    //The number of rows in best
    int bestSize = rowGroups[0][i]->size;
    //The number of rows in rest
    int restSize = rowGroups[1][i]->size;
    //Since we aren't storing the number of ranges, we need the last element to be null
    int rangeLength =  bestSize + restSize + 1;
    rangeGroups[i] = malloc(sizeof(Range *) * rangeLength);
    for(int j = 0; j < rangeLength; j++) {
      rangeGroups[i][j] = NULL;
    }
    //Only ranges in independent columns go into best/rest
    if(rowGroups[0][i]->independent) {
      for(int j = 0; j < rowGroups[0][i]->size; j++) {
        addValue(rangeGroups[i], rowGroups[0][i]->name, rowGroups[0][i]->strings[j], true, bestSize);
      }
      for(int j = 0; j < rowGroups[1][i]->size; j++) {
        addValue(rangeGroups[i], rowGroups[1][i]->name, rowGroups[1][i]->strings[j], false, restSize);
      }
    }
  }
  //We no longer need the best and rest rows
  destroyColumns(rowGroups[0]);
  destroyColumns(rowGroups[1]);
  free(rowGroups);
  return rangeGroups;
}

/**
 * Look at each range in rangeGroups and add it to a new list if the best score is greater than
 * the rest score
 * @param rangeGroups a list of unique ranges for each column
 * @return a list of ranges with bestCount > restCount
 */
Range **filter(Range ***rangeGroups) {
  int numRanges = 0;
  //Look over the whole 2D array and count the number of Range structs
  for(int i = 0; i < numColumns; i++) {
    int idx = 0;
    Range *range = rangeGroups[i][idx++];
    while(range != NULL) {
      numRanges++;
      range = rangeGroups[i][idx++];
    }
  }
  //Create a buffer which can hold, at max, every range
  Range *filterBuffer[numRanges];
  int filterIdx = 0;
  for(int i = 0; i < numColumns; i++) {
    int idx = 0;
    Range *range = rangeGroups[i][idx++];
    while(range != NULL) {
      //Only add ranges who appear more often in best than in rest
      double best = range->bestCount * 100;
      double rest = range->restCount * 100;
      if(best - rest > 0.01) {
        filterBuffer[filterIdx++] = range;
      }
      range = rangeGroups[i][idx++];
    }
    //We no longer need each range group
    free(rangeGroups[i]);
  }
  //We no longer need the whole array of range groups
  free(rangeGroups);
  //Once again, we need space for the last element to be null
  Range **filteredRanges = malloc(sizeof(Range *) * filterIdx + 1);
  filteredRanges[filterIdx] = NULL;
  for(int i = 0; i < filterIdx; i++) {
    filteredRanges[i] = filterBuffer[i];
  }
  return filteredRanges;
}

/**
 * Assigns a score to each range equivalent to b^2/(b + r)
 * @param ranges a list of ranges
 * @return the same list with scores assigned
 */
Range **assignScores(Range **ranges) {
  int idx = 0;
  Range *range = ranges[idx++];
  while(range != NULL) {
    //The output is represented as a percentage, not a decimal point
    double best = range->bestCount * 100;
    double rest = range->restCount * 100;
    range->score = (best * best) / (best + rest);
    range = ranges[idx++];
  }
  return ranges;
}

/**
 * A comparator function for qsort which compares ranges
 * @param p1 the location in memory of the first range
 * @param p2 the location in memory of the second range
 * @return <0 if p1 has a greater score than p2, >=0 otherwise
 */
int comparator(const void* p1, const void* p2) {
  Range **r1 = (Range **)p1;
  Range **r2 = (Range **)p2;
  
  return (int) (((*r2)->score - (*r1)->score) * 100);
}

/**
 * Sort the given list of ranges by score
 * @param ranges the list of ranges
 * @return the list sorted by score
 */
Range **sort(Range **ranges) {
  int length = 0;
  Range *range = ranges[length];
  while(range != NULL)
    range = ranges[++length];
  qsort(ranges, length, sizeof(Range *), comparator);
  return ranges;
}

/**
 * Print the list of ranges according to the format specified by the assignment
 * @param ranges the list of ranges
 */
void print(Range **ranges) {
  int idx = 0;
  Range *range = ranges[idx++];
  while(range != NULL) {
    printf("%d\t%.0f\t%s\t%s\t%.0f\t%.0f\n", idx, range->score, range->colName, range->value, 
           range->bestCount * 100, range->restCount * 100);
    free(range);
    range = ranges[idx++];
  }
  free(ranges);
}


/**
 * Starts the program
 * @return 0 if the program exits successfully
 */
int main() {
  print(sort(assignScores(filter(countRanges(bestRest(populate(toColumns(getLine()))))))));
}