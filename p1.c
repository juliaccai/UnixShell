 /* Julia Cai
  *  09/17/18
  *  
  *  Lexical Analyzer: takes in an input from stdin until the end of file is reached
  *  - Analyses if the input has a ' ', '\0', '\n' or EOF is reached then the previously inputted
  *  characters form a 'word' and the length of the word is returned
  *  - If there are no characters typed before the:
  *     - ' ', then ' ' is ignored
  *     - '\0' or ';' or '\n', then the character array has an empty string, 0 and is returned 
  *     - 'EOF', then the character array has an empty string, and -255 is returned
  *  - If an EOF is reached, then a -255 is returned
  *  - If any of the following metacharacters '<' '>' '|' '&' are typed then it is stored as a 'word' and a 1 is returned
  *  - If there are two consecutive '<' so '<<' then that is stored as a 'word' and 2 is returned
  *  - If the 'word' starts with a '$' symbol then the 'word' size becomes negative 
  *  - If there is a '\':
  *     - regular chars ignore it
  *     - metacharacters are printed like regular chars
  *     - newline is treated as a space ('\n')
  *  - If the word size is 254 then the word is stored and the size is returned, the rest of the word is stored as a new word
  */
   
   #include "p2.h"
   #include "getword.h"
   #include <stdlib.h>
   
   
   
   //pointer for the start of the temporary array, that will hold the initial input and the input converted 
   char typed; 
   int count = 0; //var for keeping track of how many chars are in a 'word' 
   int numchar; //temporarily holds count for when count needs to be reset and returned
   int dollar = 0; //flag for if '$' is the first char in front of a 'word'
   
   
   int getword(char *w)
   {
   //checks one char from input, proceeds rest of checks if not the end of file 
   while((scanf("%c", &typed)) != EOF)
   {
   //if the maximum storage size is reached, prints the word, then reads the rest of the word as a new word, 
   //the character is put back on the input stream with ungetc so then it can be read as part of the new word
     if(count > 1 && count%(STORAGE-1) == 0)
     {
        *(w + count) = '\0';
        numchar = count;
        count = 0;
        ungetc(typed,stdin);
        return numchar;  
     }
   
   //returns the size of the word typed before a metacharacter in order to ensure that the word preceeding the metacharacter
   //is stored as a word and its size is returned 
     if(count != 0 && (typed == '>' || typed == '<' || typed == '|' || typed == '&'))
     { 
         *(w + count)= '\0';
         numchar = count;
         count = 0;
         ungetc(typed, stdin);
         return numchar;
     }  
   
     //checks if the char is a '$', ' ', '\n', '~', ';', '<', '>', '|', '&', '\' and returns vals and edits what is in the storage depending on the specifications then if its not any of those, the char is stored
     switch (typed)
     {
   	 case '$':
   	 if(count == 0)
   	 {
   		if(dollar != 1)
   		{
   		dollar = 1;
   		break;
   		}
   	 }	
    	 *(w + count++) = '$';
     	 break;
   	
    	
   	// case '~':
   	 //adds the home path to storage, if there is a '$' in front of the '~' then '~' is printed as a regular char
   	 /*if(dollar != 1 && count == 0)
   	 {
   		 
//http://pubs.opengroup.org/onlinepubs/9699919799/functions/getdelim.html
  		 char *home = getenv("HOME");
  		 int i;
  		 for(i = 0; i < (int)strlen(home); i++)
  		 {
  			 *(w +count+i) = *(home+i);
  		 }
  		 count += strlen(home);
  		 break;
  		 
  	 } */
  	 /**(w + count++) = '~'; 
  	 break; */
  	
  
  	 case '<': 
  	 //if there are two '<' consecutively together then they print as a word, otherwise they are printed like the other metacharacters 
  	 typed = getchar();
  	 if(typed == '<')
  	 {
  	  	*w = '<';
  	  	*(w + 1) = '<';
  	  	*(w + 2) = '\0'; 
  	  	return 2; 
  	 } 		
  	else
  	{
  	 	//to put back the char into the input stream we just got from getchar(), since the character after the first '<' is not being followed by another '<', we just treat '<' like the other metacharacters and return the length as '1' and '<' as a word on it's own, and then the other character needs to be read again to see if it fits any of the cases in the switch statement 
  	 	 ungetc(typed,stdin);
  	  	 *w = '<';
           *(w + 1) = '\0';
           return 1;
  	}
  
  
  	 case '>':
  	 case '|': 
  	 case '&':
  	  *w = typed;
  	  *(w + 1) = '\0';
        return 1;
  
  
       case ' ':
   	 if(count == 0)
   	   break;
  	 numchar = count;
  	 *(w + count) = '\0';
  	 count = 0;
  	 if(dollar == 1)
  	 {
   	   dollar = 0;
  	   return -1*numchar;
  	 }
  	 return numchar;
  
  
       case '\n':
       case ';':
  	 if(dollar == 1)
  	 {
  	    dollar = 0;		
  	    count =  -1*count;
  
  	    //puts the last char analyzed back into stdin to be analyzed again
  	    //accounts for the '$' when a different end of word character is encountered
  	    ungetc(typed,stdin);
         	    numchar = count;
         	    *(w + abs(count)) = '\0';
         	    count = 0;
              return numchar;
  	 }
  
  	 if(count == 0)
  	 {
  	     *w = '\0';
  	     return count;
   	 }
  	 //puts the last char analyzed back into stdin to be analyzed again and accounted for 
  	 ungetc(typed,stdin);
  	 numchar = count;
  	 *(w + abs(count)) = '\0';
  	 count = 0;
  	 return numchar; 
  
  	 	//if a backslash is typed then it is followed by a '\n' then a ' ' is put back in stdin
  	//if there is a backslash in front of the EOF then the backslash is just ignored and EOF is pushed back to the input stream to allow the program to return EOF functionalities correctly as previously specified
  	case '\\':
  	{
   	   typed = getchar();
             if(typed == '\n')
             {
                ungetc(' ', stdin);
                break;
             }
  		   if(typed == ';')
             {
  			  //counts ";" as a regular char if preceeded by a backslash
                *(w + count++) = typed;
                break;
             }
             if(typed == EOF)
             {
                ungetc(EOF, stdin);
                break;
             }
  		   flag_backslash = 1;
  	}
      default:
  	//for when a char is typed	
  	*(w + count++) = typed;
  	break;
    }
  }
  
  //ensures the last word was null terminated
  *(w + abs(count)) = '\0';
  //makes sure the last word's length was returned
  while(abs(count) > 0)
  {
    if(dollar == 1)
    {
  	  dollar = 0;
  	  count = -1*count;	
    }
    numchar = count;
    count = 0;
    return numchar;
  }
  
  if(dollar == 1)
  {
      dollar = 0;
  
      //puts the EOF analyzed back into stdin to be analyzed again
      //accounts for the '$' when EOF is encountered
      ungetc(EOF,stdin);
      *w = '\0';
      return count = 0;
  }
  
  
  //if the end of file is reached, then the empty array is printed and a -255 is returned, returning a -255 exits out of program
  *w = '\0';
  return -255;
} 
