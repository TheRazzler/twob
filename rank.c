#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define VALUE_DEFAULT 100

/** The number of columns */
int numColumns = 0;

/** Stores the name of a column and a dynamically resizable array of either numbers or strings */
typedef struct ColumnStruct {
  char name[50];
  bool isNumber;
  int size; //Number of elements in the array
  int length; //Length of the array
  double *numbers;
  char **strings;
} Column;

/**
 * Constructs a new column with the given name and isNumber value
 * @param name the name of the column
 * @param isNumber whether or not the column holds numbers
 * @return a pointer to the constructed column
 */
Column *newColumn(char name[50], bool isNumber) {
  Column *c = malloc(sizeof(Column));
  strcpy(c->name, name);
  c->isNumber = isNumber;
  c->size = 0;
  c->length = VALUE_DEFAULT;
  if(isNumber) {
    c->numbers = malloc(sizeof(double) * VALUE_DEFAULT);
    c->strings = NULL;
  } else {
    c->numbers = NULL;
    c->strings = malloc(sizeof(char *) * VALUE_DEFAULT);
  }
  return c;
}

/**
 * Adds a numerical value to the given column
 * @param c the given column
 * @param value the value to add
 * @return true if the value could be added to the column
 */
bool addNumber(Column *c, double value) {
  if(c->isNumber) {
    if(c->size == c->length) {
      double *copy = malloc(sizeof(double) * c->length * 2);
      for(int i = 0; i < c->length; i++) {
        copy[i] = c->numbers[i];
      }
      free(c->numbers);
      c->numbers = copy;
    }
    c->numbers[c->size++] = value;
    return true;
  }
  return false;
}

/**
 * Adds a string to the given column
 * @param c the given column
 * @param value the string to add
 * @return true if the value could be added to the column
 */
bool addString(Column *c, char *value) {
  if(!c->isNumber) {
    if(c->size == c->length) {
      char **copy = malloc(sizeof(char *) * c->length * 2);
      for(int i = 0; i < c->length; i++) {
        copy[i] = c->strings[i];
      }
      free(c->strings);
      c->strings = copy;
    }
    c->strings[c->size++] = value;
    return true;
  }
  return false;
}

/**
 * @return a line from standard input
 */
char *getLine() {
  char *buffer = malloc(sizeof(char) * 200);
  int c = getchar();
  int i;
  for(i = 0; i < 199; i++) {
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
  char name[50];
  Column **columns = malloc(sizeof(Column *) * 50);
  for(;;) {
    if(c == '\0') {
      name[nameIdx] = '\0';
      columns[numColumns++] = newColumn(name, name[0] == '>' || name[0] == '<' || name[0] == '$');
      //Check for debugging
      if(numColumns >= 50) {
        printf("ERROR: NUM COLUMNS EXCEDING BUFFER SIZE\n");
        exit(1);
      }
      break;
    }
    if(c == ',') {
      name[nameIdx] = '\0';
      columns[numColumns++] = newColumn(name, name[0] == '>' || name[0] == '<' || name[0] == '$');
      nameIdx = 0;
      //Check for debugging
      if(numColumns >= 50) {
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
 * Starts the program
 * @return 0 if the program exits successfully
 */
int main() {
  Column **columns = toColumns(getLine());
  for(int i = 0; i < numColumns; i++) {
    printf("%s\n", columns[i]->name);
  }
}