#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

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
  /** The value for this range */
  char value[COL_WIDTH_BUFFER];
  /** The weighted number of appearances in best */
  double bestCount = 0;
  /** The weighted number of appearances in rest */
  double restCount = 0;
} Range;

/**
 * Constructs a new column with the given name
 * @param name the name of the column
 * @return a pointer to the constructed column
 */
Column *newColumn(char name[COL_WIDTH_BUFFER]) {
  Column *c = malloc(sizeof(Column));
  strcpy(c->name, name);
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
      free(c->strings[i]);
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
 * @return true if the value could be added to the column
 */
bool addString(Column *c, char *value) {
  if(c->size == c->length) {
    char **copy = malloc(sizeof(char *) * c->length * 2);
    for(int i = 0; i < c->length; i++) {
      copy[i] = malloc(sizeof(char) * COL_WIDTH_BUFFER);
      strcpy(copy[i], c->strings[i]);
    }
    free(c->strings);
    c->strings = copy;
  }
  c->strings[c->size] = malloc(sizeof(char) * COL_WIDTH_BUFFER);
  strcpy(c->strings[c->size++], value);
  return true;
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
  Column **columns = malloc(sizeof(Column *) * COL_WIDTH_BUFFER);
  for(;;) {
    if(c == '\0') {
      name[nameIdx] = '\0';
      columns[numColumns++] = newColumn(name);
      //Check for debugging
      if(numColumns >= COL_WIDTH_BUFFER) {
        printf("ERROR: NUM COLUMNS EXCEDING BUFFER SIZE\n");
        exit(1);
      }
      break;
    }
    if(c == ',') {
      name[nameIdx] = '\0';
      columns[numColumns++] = newColumn(name);
      nameIdx = 0;
      //Check for debugging
      if(numColumns >= COL_WIDTH_BUFFER) {
        printf("ERROR: NUM COLUMNS EXCEDING BUFFER SIZE\n");
        exit(1);
      }
    } else {
      name[nameIdx++] = c;
    }
    c = names[idx++];
  }
  Column **final = malloc(sizeof(Column *) * numColumns);
  for(int i = 0; i < numColumns; i++) {
    final[i] = columns[i];
  }
  free(columns);
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
  Column *klass = columns[numColumns - 1];
  char oneVal[COL_WIDTH_BUFFER];
  strcpy(oneVal, klass->strings[0]);
  for(int i = 1; i < klass->size; i++) {
    if(strcmp(oneVal, klass->strings[i]) != 0) {
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
  for(int i = 0; i < numColumns; i++) {
    best[i] = newColumn(columns[i]->name);
    rest[i] = newColumn(columns[i]->name);
  }
  
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
  
  destroyColumns(columns);
  Column ***bestRest = malloc(sizeof(Column **) * 2);
  bestRest[0] = best;
  bestRest[1] = rest;
  return bestRest;
}

Range ***countRanges(Column ***rowGroups) {
  Range ***rangeGroups = 
}


/**
 * Starts the program
 * @return 0 if the program exits successfully
 */
int main() {
  Column ***groups = bestRest(populate(toColumns(getLine())));
  for(int i = 0; i < 2; i++) {
    for(int j = 0; j < groups[i][0]->size; j++) {
      printf("%s", groups[i][0]->strings[j]);
      for(int k = 1; k < numColumns; k++) {
        printf(",%s", groups[i][k]->strings[j]);
      }
      printf("\n");
    }
  }
  
  destroyColumns(groups[0]);
  destroyColumns(groups[1]);
  free(groups);
}