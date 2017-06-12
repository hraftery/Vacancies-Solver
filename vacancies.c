#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <sys/mman.h>

#include <errno.h>
#include <string.h>

//For fork()
#include <unistd.h>
//For clock()
#include <time.h>
//For gettimeofday()
#include <sys/time.h>


//--
//Type definitions
//--

typedef enum CardSuit
{
	csMIN		= 0,
	csHearts	= csMIN,
	csDiamonds,
	csClubs,
	csSpades,
	csMAX,
	csAny
} eCardSuit;

typedef struct Card
{
	int suit;
	int rank;
} sCard;

typedef struct Loc
{
    int row;
    int col;
} sLoc;

typedef struct Move
{
	sLoc src;
	sLoc dst;
} sMove;


//--
//Defines
//--
#define NUM_ROWS	4
#define NUM_COLS	10
#define DECK_SIZE	(csMAX*10)

//maps every card to a unique 0=based index in gLocs
#define LOC_IDX_FOR_CARD(c)     (c.suit*10 + c.rank - 1)

#define PROGRESS_DEPTH  10


//--
//Globals
//--
sCard DECK[DECK_SIZE];
sCard GRID[NUM_ROWS][NUM_COLS];

sLoc gLocs[DECK_SIZE];

int *gpMaxScore; //Pointer so we can share between processes
sMove gCurrentMoves[100];
sMove gMaxMoves[100];
int gMaxDepth;

struct timeval gStartTime;

int gProgress[PROGRESS_DEPTH];


//--
//Forward declarations
//--
int FindMoves(sMove moves[]);
void TakeTurn(sMove move);
void MakeMove(sMove move);
void PrintGrid(void);
int CalcScore(void);


//--
//Function definitions
//--
int main(int argc, char *argv[])
{
	int i, j;
	
	sranddev();
	
    //Init deck of cards
	for(i=csMIN; i<csMAX; i++)
	{
		for(j=1; j<=10; j++)
		{
			sCard c;
			c.suit = i;
			c.rank = j;
			DECK[LOC_IDX_FOR_CARD(c)] = c;
		}
	}

    //Init grid randomly from deck
	for(i=0; i<NUM_ROWS; i++)
	{
		for(j=0; j<NUM_COLS;)
		{
			int k = rand() % DECK_SIZE;
			if(DECK[k].rank == 0)
				continue;
			
			GRID[i][j] = DECK[k];
            
            gLocs[LOC_IDX_FOR_CARD(DECK[k])].row = i;
            gLocs[LOC_IDX_FOR_CARD(DECK[k])].col = j;
            
			DECK[k].rank = 0;
			j++;
		}
	}
	
	PrintGrid();

    //Init globals maxScore and startTime
    gpMaxScore = mmap(NULL, sizeof(*gpMaxScore), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	*gpMaxScore = 0;
    gettimeofday(&gStartTime, NULL);
    
    //Get initial set of available moves
	sMove moves[16]; //can be up to 16 moves if all 4 Aces are in the first column
	int numMoves = FindMoves(moves);
    
    pid_t pids[16];
	for(i=0; i<numMoves; i++)
	{
        if((pids[i] = fork()) < 0)
        {
            printf("\nERROR: fork.\n");
            exit(1);
        }
        else if(pids[i] == 0) //child
        {
            gProgress[0] = i;
            TakeTurn(moves[i]); //each child works on one of the initial moves
            exit(0);
        }
	}

    //Parent waits for all children to finish
    for(i=0; i<numMoves; i++)
    {
        int status;
        pid_t pid = wait(&status);
        if(pid == -1)
        {
            printf("Got error %d while waiting for children: %s\n", errno, strerror(errno));
            break;
        }
        printf("Child with PID %d exited with status 0x%x.\n", pid, status);
        if(status == 1) // indicates we should terminate
            break;
    }
    
    printf("\nBest score: %d\n", *gpMaxScore);
    for(i=0; i<gMaxDepth; i++)
    {
        printf("%d%d-%d%d;", gMaxMoves[i].src.row, gMaxMoves[i].src.col,
                             gMaxMoves[i].dst.row, gMaxMoves[i].dst.col);
    }
    printf("\n");
    
	return 0;
}

void TakeTurn(sMove move)
{
    /* Profiling
    static int turnCount=0, turnCountCount=0;
    static clock_t lastTime = 0;
    
    if(++turnCount == 10000000)
    {
        clock_t now = clock();
        
        turnCountCount++;
        printf("%d: another 10,000,000 turns.\n", turnCountCount);
        printf("Took: %ldms.\n", (now-lastTime)/1000);
        
        turnCount = 0;
        lastTime = now;
    }
    */
    static int depth=0;
    
    gCurrentMoves[depth] = move;
    MakeMove(move);
    depth++;

    if(depth == 100)
    {
        printf("** Depth reached 100. Abandoning. **");
        exit(1);
    }
    
	sMove moves[16]; //can be up to 16 moves if all 4 Aces are in the first column
	int i, j, numMoves = FindMoves(moves);

	for(i=0; i<numMoves; i++)
	{
        if(depth<PROGRESS_DEPTH)
            gProgress[depth] = i;
        if(depth==PROGRESS_DEPTH-1)
        {
            printf("Progress: ");
            for(j=0; j<PROGRESS_DEPTH; j++)
                printf("%d", gProgress[j]);
            printf(".\n");
        }
        
    	TakeTurn(moves[i]);
	}
	
	if(numMoves == 0)
	{
        /*
        for(i=depth>20 ? 15 : 0; i<depth; i++)
        {
            printf("%d%d-%d%d;", gCurrentMoves[i].srcRow, gCurrentMoves[i].srcCol,
                                 gCurrentMoves[i].dstRow, gCurrentMoves[i].dstCol);
        }
        printf("\n");
        */
        
		int score = CalcScore();
		if(score > *gpMaxScore)
		{
            struct timeval nowTime;
            
			*gpMaxScore = score;
            for(i=0; i<depth; i++)
                gMaxMoves[i] = gCurrentMoves[i];
            gMaxDepth = depth;
            
            gettimeofday(&nowTime, NULL);
            
			printf("New max score: %d. Time elapsed: %ld.\n", *gpMaxScore, nowTime.tv_sec - gStartTime.tv_sec);
            for(i=0; i<depth; i++)
            {
                printf("%d%d-%d%d,", gCurrentMoves[i].src.row, gCurrentMoves[i].src.col,
                                     gCurrentMoves[i].dst.row, gCurrentMoves[i].dst.col);
            }
            printf("\n");
            
			PrintGrid();
            
            if(*gpMaxScore == 36) //we're done
                exit(1);
		}
	}
    
    //undo turn
    MakeMove(move);

    depth--;
}

void MakeMove(sMove move)
{
    sCard cDst = GRID[move.dst.row][move.dst.col];
    sCard cSrc = GRID[move.src.row][move.src.col];
    GRID[move.dst.row][move.dst.col] = GRID[move.src.row][move.src.col];
    GRID[move.src.row][move.src.col] = cDst;
    
    sLoc l = gLocs[LOC_IDX_FOR_CARD(cDst)];
    gLocs[LOC_IDX_FOR_CARD(cDst)] = gLocs[LOC_IDX_FOR_CARD(cSrc)];
    gLocs[LOC_IDX_FOR_CARD(cSrc)] = l;
}

int FindMoves(sMove moves[])
{
	int i, j;
	int numMoves=0;
	
    sCard ace;
    ace.rank = 1; //look for each of the aces
	for(ace.suit=csMIN; ace.suit<csMAX; ace.suit++)
	{
        i = gLocs[LOC_IDX_FOR_CARD(ace)].row;
        j = gLocs[LOC_IDX_FOR_CARD(ace)].col;
        
        //Find source cards. There are 3 possibilities.
        if(j == 0) //Possibility 1: any of the four 2's will do, unless they're already in col 0.
        {
            sCard two;
            two.rank = 2;
            for(two.suit=csMIN; two.suit<csMAX; two.suit++)
            {
                sLoc twoLoc = gLocs[LOC_IDX_FOR_CARD(two)];
                if(twoLoc.col == 0)
                    continue;
                moves[numMoves].dst.row = i;
                moves[numMoves].dst.col = j;
                moves[numMoves].src     = twoLoc;
                numMoves++;
            }
        }
        else if(GRID[i][j-1].rank > 1 && GRID[i][j-1].rank < 10) //Possibility 2: only one particular card will do
        {
            sCard c = GRID[i][j-1]; //get card to the left
            c.rank++; //the card we're looking for is same suit, one rank higher
            
            moves[numMoves].dst.row = i;
            moves[numMoves].dst.col = j;
            moves[numMoves].src     = gLocs[LOC_IDX_FOR_CARD(c)];
            numMoves++;
        }
        //Possibility 3, we're next to an Ace or a Ten, so no card will do.
	}
	
	return numMoves;
}

void PrintGrid(void)
{
	int i, j;
	
	for(i=0; i<NUM_ROWS; i++)
	{
		for(j=0; j<NUM_COLS; j++)
		{
			if(GRID[i][j].rank == 1)
				printf(".. ");
			else
			{
				char c;
				switch(GRID[i][j].suit)
				{
					case 0: c = 'H'; break;
					case 1: c = 'D'; break;
					case 2: c = 'C'; break;
					case 3: c = 'S'; break;
                    default: printf("\n**ERROR**\n"); c = 'X'; break;
				}
				printf("%d%c ", GRID[i][j].rank == 10 ? 0 : GRID[i][j].rank, c);
			}
		}
		printf("\n");
	}
	printf("\n");
}

int CalcScore(void)
{
	int i, j;
	int score=0;
	
	for(i=0; i<NUM_ROWS; i++)
	{
		for(j=0; j<NUM_COLS; j++)
		{
			if(GRID[i][j].rank == j+2)
				score++;
			else
				break;
		}
	}
	
	return score;
}
