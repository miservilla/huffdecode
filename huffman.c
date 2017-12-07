/*******************************************************************************
 * Michael Servilla, huffman.c is code to take in a file stream, analyze it by
 * developing a character frequency list, creating a priority queue based on
 * this frequency, building a binary tree based on huffman algorithm, and then
 * using the prefix codes to encode(compress) the file. Memory leak and seg
 * with decoding evil.jpg.huff are ongoing issues that I would attempt to
 * given enough time.
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include "huffman.h"

struct HuffData/*Struct to build both list and tree nodes.*/
{
  int symbol;/*The ascii value of each character from the input file.*/
  unsigned long frequency;/*The frequency of each character*/
  struct HuffData *next;/*The next pointer when list is used.*/
  struct HuffData *left;/*The left pointer when tree is used.*/
  struct HuffData *right;/*The right pointer when tree is used.*/
};

/*Begin global variables.*/
unsigned long frequencyCount[256];
char *lookUpTable[256];
unsigned long charCount = 0;
char *binCode;
int binCodeLength[256];
int counter;
unsigned char returnChar;
unsigned char totalSymbol = 0;
int count;
struct HuffData *originalHead;
struct HuffData *walkingNode;
int byte;

/*Begin function prototypes.*/
void getCodeRecursion(struct HuffData*, char [], int, int);
struct HuffData* insertSorted(struct HuffData*, int,
                              unsigned long, struct HuffData*,
                              struct HuffData*);
struct HuffData* makeList(unsigned long []);
struct HuffData* makeTree(struct HuffData*);
void buildCode(int, char [], int);
void printStdOut();
void encode(int , FILE*);
void bitWise(char [], int, FILE*);
void printHeader(FILE*);
struct HuffData* readHeader(struct HuffData*, FILE*);
struct HuffData* walkTree(struct HuffData*, unsigned char byte, FILE*);
void encodeFile(FILE*, FILE*);
void decodeFile(FILE*, FILE*);

/*******************************************************************************
 * createNode takes 4 arguments, an int, an unsigned long, and two pointers
 * which are used when creating a Huffman tree. Otherwise the left and right
 * pointers are ignored when working with list. A HuffData struct is returned.
 ******************************************************************************/
struct HuffData* createNode(int symbol, unsigned long frequency,
                            struct HuffData* left, struct HuffData* right)
{
  struct HuffData* newListNode = (struct HuffData*)
          malloc(sizeof(struct HuffData));
  newListNode->symbol = symbol;
  newListNode->frequency = frequency;
  newListNode->next = NULL;
  newListNode->left = left;
  newListNode->right = right;
  return newListNode;
}

/*******************************************************************************
 * insertSorted takes 5 arguments, a struct pointer to head, an int, an unsigned
 * long, and two pointers which are used when creating and navigating the
 * Huffman tree. It returns a HuffData struct in either the form of a sorted
 * linked list, or Huffman tree.It uses multiple conditionals and assignments.
 ******************************************************************************/
struct HuffData* insertSorted(struct HuffData* head, int symbol,
                              unsigned long frequency, struct HuffData* left,
                              struct HuffData* right)
{
  struct HuffData* current;
  struct HuffData* temp = head;
  struct HuffData* temp2;

  if(head == NULL)
  {
    current = createNode(symbol, frequency, left, right);
    current->next = head;
    head = current;
    return head;
  }
  if(frequency < temp->frequency)
  {
    current = createNode(symbol, frequency, left, right);
    current->next = head;
    head = current;
    return head;
  }
  if(frequency == temp->frequency)
  {
    if(symbol < temp->symbol)
    {
      current = createNode(symbol, frequency, left, right);
      current->next = head;
      head = current;
      return head;
    }
    while(symbol > temp->symbol && frequency == temp->frequency)
    {
      if(temp->next == NULL)
      {
        current = createNode(symbol, frequency, left, right);
        temp->next = current;
        return head;
      }
      temp2 = temp;
      temp = temp->next;
    }
    current = createNode(symbol, frequency, left, right);
    current->next = temp;
    temp2->next = current;
    return head;
  }
  while(frequency > temp->frequency)
  {
    if(temp->next == NULL)
    {
      current = createNode(symbol, frequency, left, right);
      temp->next = current;
      return head;
    }
    temp2 = temp;
    temp = temp->next;
  }
  while(symbol > temp->symbol && frequency == temp->frequency)
  {
    if(temp->next == NULL)
    {
      current = createNode(symbol, frequency, left, right);
      temp->next = current;
      return head;
    }
    temp2 = temp;
    temp = temp->next;
  }
  current = createNode(symbol, frequency, left, right);
  current->next = temp;
  temp2->next = current;
  return head;
}

/*******************************************************************************
 * makeList takes an array of unsigned long values. It traverses the list and if
 * if any of the values are greater than 0 (indicates character was stored
 * there), it will pass to insertSorted in order to make a sorted linked list.
 * A HuffData struct is returned. Note, NULL values for left and right children
 * which are not utilized for linked list.
 ******************************************************************************/
struct HuffData* makeList(unsigned long array[])
{
  int i;
  struct HuffData* head = NULL;

  for(i = 0; i < 256; ++i)
  {
    if(array[i] > 0)
    {
      charCount += array[i];
      head = insertSorted(head, i, array[i], NULL, NULL);
    }
  }
  return head;
}

/*******************************************************************************
 * makeTree takes an already created linked list and uses Huffman algorithm to
 * build Huffman tree. It returned a HuffData struct in form of tree.
 ******************************************************************************/
struct HuffData* makeTree(struct HuffData* head)
{
  while(head->next != NULL)
  {
    struct HuffData* current;
    current = createNode(head->symbol, head->frequency + head->next->frequency,
                         NULL, NULL);
    current->left = head;
    current->right = head->next;
    if(head->next->next != NULL)
    {
      head = head->next->next;
    }
    head = insertSorted(head, current->symbol, current->frequency,
                        current->left, current->right);
  }
  return head;
}

/*******************************************************************************
 * getCodeRecursion takes a HuffData struct, the Huffman tree, a char array, and
 * two ints, and recursively traverses entire tree noting left or right
 * direction as it goes down the tree looking for leaf nodes. When a leaf node
 * is reached, it then calls buildCode function to store code in a 2 dimension
 * array (lookUpTable). It returns void.
 ******************************************************************************/
void getCodeRecursion(struct HuffData* head, char code[], int edgeCount,
                      int direction)
{
  if(head == NULL)
  {
    return;
  }

  if(direction != 'R')
  {
    code[edgeCount] = direction;
    edgeCount++;
  }


  if(head->left == NULL && head->right == NULL)
  {
    buildCode(head->symbol, code, edgeCount);
  }
  else
  {
    getCodeRecursion(head->left, code, edgeCount, '0');
    getCodeRecursion(head->right, code, edgeCount, '1');
  }
}

/*******************************************************************************
 * buildCode takes an ascii value as an int, a char array, and an int value of
 * number of edges traversed down to the particular leaf. It stores the char
 * array in the location on the lookUpTable at the index corresponding to the
 * ascii value. It returns void.
 ******************************************************************************/
void buildCode(int symbol, char code[], int edgeCount)
{
  int i;
  totalSymbol++;
  lookUpTable[symbol] = malloc(sizeof(char) * edgeCount);
  for(i = 0; i < edgeCount; i++)
  {
    lookUpTable[symbol][i] = code[i];
  }
  binCodeLength[symbol] = edgeCount;
}

/*******************************************************************************
 * printStdOut takes no arguments and returns void. It loops through an array
 * prints out data including the character symbol, frequency of occurrences, and
 * the associated prefix code.
 ******************************************************************************/
void printStdOut()
{
  int i;
  int j = 0;
  printf("Symbol\tFreq\tCode\n");
  for(i = 0; i < 256; ++i)
  {
    if(frequencyCount[i] > 0)
    {
      j++;
      if(!(i > 32 && i < 132))
        printf("=%d\t%lu\t%s\n", i, frequencyCount[i], lookUpTable[i]);
      else
      {
        printf("%c\t%lu\t%s\n", i, frequencyCount[i], lookUpTable[i]);
      }
    }
  }
  printf("Total chars = %lu\n", charCount);
}

/*******************************************************************************
 * maxDepth takes a HuffData struct (the tree) and returns the maximum depth
 * value of the tree.
 ******************************************************************************/
int maxDepth(struct HuffData* head)
{
  if(!head)
  {
    return 0;
  }
  else
  {
    int leftDepth = maxDepth(head->left);
    int rightDepth = maxDepth(head->right);
    if(leftDepth > rightDepth)
    {
      return leftDepth + 1;
    }
    else
    {
      return rightDepth + 1;
    }
  }
}

/*******************************************************************************
 * encode takes an int value representing the ascii value of the particular
 * character read in, and a destination output file. It passes bother on to
 * the function that does bitwise manipulation and creates a byte stream of
 * prefix codes. It returns void.
 ******************************************************************************/
void encode(int c, FILE* out)
{
  binCode = (char*) malloc(sizeof(char) * binCodeLength[c]);
  binCode = lookUpTable[c];
  bitWise(binCode, binCodeLength[c], out);
}

/*******************************************************************************
 * bitWise take in a char array of one prefix code, and int of the code length,
 * and a designated output file. It will grab the least bit data of on character
 * of code, and OR it to a waiting byte. A for loop is used to adjust shifting
 * of the least bit to necessary location before it is ORed to the awaiting
 * byte (returnChar). When this byte is full, it is then written to the output
 * file. Void is returned.
 ******************************************************************************/
void bitWise(char binArray[], int arrayLength, FILE* out)
{
  int i;
  unsigned char bit = 0;
  for(i = 0; i < arrayLength; i++)
  {
    bit = (char) (binArray[i] - '0');
    bit = bit << counter--;
    returnChar = returnChar | bit;
    if(counter + 1 == 0)
    {
      fputc(returnChar, out);
      counter = 7;
      returnChar = 0;
    }
  }
}

/*******************************************************************************
 * printHeader takes a designated output file and prints necessary information
 * to this file including total symbol count, symbol and associated prefix code,
 * and total character count. It returns void.
 ******************************************************************************/
void printHeader(FILE *out)
{
  int i;
  fwrite(&totalSymbol, 1, 1, out);
  for(i = 0; i < 256; ++i)
  {
    if(frequencyCount[i] > 0)
    {
      fwrite(&i, 1, 1, out);
      fwrite(&frequencyCount[i],8, 1, out);
    }
  }
  fwrite(&charCount, 8, 1, out);
}

/*******************************************************************************
 * readHeader takes in a HuffData struct and FILE, this calls insertSorted to
 * build a priority queue. It returns a HuffData struct.
 ******************************************************************************/
struct HuffData* readHeader(struct HuffData* head, FILE* in)
{
  int j;
  int i;
  fread(&totalSymbol,1,1,in);
  for(j = 0; j < totalSymbol; j++)
  {
    fread(&i,1,1,in);
    fread(&frequencyCount[i],8,1,in);
    head = insertSorted(head,i,frequencyCount[i],NULL,NULL);
  }
  fread(&charCount,8,1,in);
  return head;
}

/*******************************************************************************
 * walkTree takes 3 arguments including a HuffData struct, unsigned char, and
 * FILE. It recursively traverses the Huffman tree based on the obtained prefix
 * byte code and when reaches the appropriate leaf, sends the character to file.
 * It returns a HuffData struct.
 ******************************************************************************/
struct HuffData* walkTree(struct HuffData *head, unsigned char byte, FILE *out)
{
  unsigned char bit;
  unsigned char mask = 128;
  walkingNode = head;

  if(walkingNode->left == NULL && walkingNode->right == NULL)
  {
    counter++;
    if(counter < charCount + 1)
    {
      fputc(walkingNode->symbol, out);
    }
    head = originalHead;
    walkTree(head, byte, out);
  }
  if(count == 8)
  {
    return walkingNode;
  }
  count++;
  bit = byte & mask;
  bit = bit >> 7;
  if(bit == 1)
  {
    walkingNode = walkingNode->right;
    byte = byte << 1;
    walkTree(walkingNode, byte, out);
  }
  else
  {
    walkingNode = walkingNode->left;
    byte = byte << 1;
    walkTree(walkingNode, byte, out);
  }
  return walkingNode;
}

/*******************************************************************************
 * freeTree is a takes a HuffData struct (the tree) and frees each node. It
 * returns void.
 ******************************************************************************/
void freeTree(struct HuffData* head)
{
  if(head)
  {
    freeTree(head->left);
    freeTree(head->right);
    free(head);
  }
}

/*******************************************************************************
 * encodeFile is the pseudo main function that takes one input and one output
 * FILE, and drives the remainder of the code. It returns void.
 ******************************************************************************/
void encodeFile(FILE *in, FILE *out)
{
  int c;
  int i;
  int edgeCount = 0;
  int height;
  char *code;



  struct HuffData* head = NULL;/*Declares a HuffData struct, head.*/

  for(i = 0; i < 256; ++i)/*Cleans array and sets all to 0.*/
  {
    frequencyCount[i] = 0;
  }

  while((c = fgetc(in)) != EOF)/*Reads character from file and determines the
                                 frequency of that character.*/
  {
    if(c == 256)
    {
      frequencyCount[0]++;
    }
    frequencyCount[c]++;
  }


  head = makeList(frequencyCount);
  head = makeTree(head);
  height = maxDepth(head);
  code = malloc(sizeof(char) * (height));
  getCodeRecursion(head, code, edgeCount, 'R');
  printStdOut();
  printHeader(out);
  rewind(in);
  counter = 7;
  returnChar = 0;
  while((c = fgetc(in)) != EOF)
  {
    encode(c, out);
  }
  bitWise("0000000", 7, out);
  freeTree(head);
  free(code);
  free(binCode);
}

/*******************************************************************************
 * decodeFile is the pseudo main function that takes one input and one output
 * FILE, and drives the remainder of the code. It returns void.
 ******************************************************************************/
void decodeFile(FILE *in, FILE *out)
{
  struct HuffData *head;
  count = 0;
  counter = 0;
  head = NULL;

  head = readHeader(head, in);
  head = makeTree(head);
  originalHead = head;

  while(EOF != (byte = fgetc(in)))/*Grabs byte from file.*/
  {
    count = 0;
    head = walkTree(head, byte, out);
  }
}