#include <cstdio>
#include <cstdlib>
#include <string.h>
#include <ctime>
#include <cmath>
#include <vector>
#include <queue>
#include <mpi.h>

#define ALIVE 'X'
#define DEAD '.'

using std::cerr;
using std::endl;

struct cell
{
	int w, h, value;
	void print() const
	{
		cerr << "cell" << "(" << w << ", " << h << ", " << value << ")" << endl;
	}
};

MPI::Datatype cell_type;

int toindex(int row, int col, int N) {
  if (row < 0) {
    row = row + N;
  } else if (row >= N) {
    row = row - N;
  }
  if (col < 0) {
    col = col + N;
  } else if (col >= N) {
    col = col - N;
  }
  return row * N + col;
}

void printgrid(char* grid, FILE* f, int N) {
  for (int i = 0; i < N; ++i) {
  	for (int j = 0; j < N; ++j) {
  		fprintf(f, "%c", grid[i * N + j]);
  	}
  	printf("\n");
  }
}

struct ProcessWorkInfo
{
	ProcessWorkInfo(int id, int N, int wChunks, int hChunks): id(id), N(N), wChunks(wChunks), hChunks(hChunks)
	{
		wSize = (N + wChunks - 1) / wChunks;
		hSize = (N + hChunks - 1) / hChunks;
		int wPos, hPos;
		getPosById(id, wPos, hPos);
		uH = hPos * hSize;
		dH = std::min(uH + hSize, N);
		lW = wPos * wSize;
		rW = std::min(lW + wSize, N);
		myHSize = dH - uH;
		myWSize = rW - lW;
		cerr << id << ": " << "My size: " << "(" << myHSize << ", " << myWSize << ")" << endl;
		cerr << id << ": " << "My cell: " << "(" << hPos << ", " << wPos << ")" << endl;
		cerr << id << ": " << "My box: (" << uH << ", " << lW << ") (" << dH << ", " << rW << ")" << endl;
		int size = (myWSize + 2) * (myHSize + 2);
		cerr << id << ": " << "size = " << size << endl;
		grid = (char*) malloc(size * sizeof(char));
		buf = (char*) malloc(size * sizeof(char));
		cerr << id << ": " << "Receiving grid" << endl;
		for (int h = uH - 1; h < dH + 1; ++h)
		{
			for (int w = lW - 1; w < rW + 1; ++w)
			{
				int nh = h;
				int nw = w;
				int cellID = cellId(nh, nw);
				if (cellID >= size)
				{
					cerr << id << ": " << "Bad cell id " << cellID << ": " << "(" << h << ", " << w << ")" << endl;
					continue;
				}
				requests.push(MPI::COMM_WORLD.Irecv(grid + cellID, 1, MPI::UNSIGNED_CHAR, 0, 0));
			}
		}
		while (!requests.empty())
		{
			requests.front().Wait();
			requests.pop();
		}
		cerr << id << ": " << "Grid received" << endl;
	}

	~ProcessWorkInfo()
	{
		free(grid);
		free(buf);
	}

	void updateGrid()
	{
		for (int j = lW - 1; j < rW + 1; ++j)
		{
			recvCell(uH - 1, j);
			recvCell(dH, j);
		}
		for (int i = uH - 1; i < dH + 1; ++i)
		{
			recvCell(i, lW - 1);
			recvCell(i, rW);
		}

    for (int i = uH; i < dH; ++i) 
    {
      for (int j = lW; j < rW; ++j) 
      {
        int alive_count = 0;
        for (int di = -1; di <= 1; ++di) 
        {
          for (int dj = -1; dj <= 1; ++dj) 
          {
            if ((di != 0 || dj != 0) && grid[cellId(i + di, j + dj)] == ALIVE) 
            {
              ++alive_count;
            }
          }
        }
        int current = cellId(i, j);
        if (alive_count == 3 || (alive_count == 2 && grid[current] == ALIVE)) 
        {
          buf[current] = ALIVE;
        } else {
          buf[current] = DEAD;
        }

        sendIfRequired(i, j, buf[current]);
      }
    }     
    cerr << id << ": " << "Sended " << sendRequests.size() << endl;
    cerr << id << ": " << "Waiting for " << recvRequests.size() << endl;
    while (!recvRequests.empty())
    {
    	recvRequests.front().first.Wait();
    	cell C = *recvRequests.front().second;
    	buf[cellId(C.h, C.w)] = C.value;
    	recvRequests.pop();
    }
    while (!sendRequests.empty())
    {
    	sendRequests.front().Wait();
    	sendRequests.pop();
    }

   	char* tmp;
   	tmp = grid;
   	grid = buf;
   	buf = tmp; 
	}

	void sendIfRequired(int i, int j, char value)
	{
    for (int di = -1; di <= 1; ++di) 
    {
      for (int dj = -1; dj <= 1; ++dj) 
      {
      	int ni = i + di;
      	int nj = j + dj;
      	if ((ni < uH) || (ni >= dH) || (nj < lW) || (nj >= rW))
      	{
      		sendCell(i, j, di, dj, value);
      	}
      }
    }
  }

  void normalize(int& i, int& j) const
  {
  	if (i < 0)
  		i += N;
  	if (j < 0)
  		j += N;

  	if (i >= N)
  		i -= N;
  	if (j >= N)
  		j -= N;
  }

  void sendCell(int i, int j, int di, int dj, char value)
  {
  	int ni = i + di;
  	int nj = i + dj;
  	normalize(ni, nj);
  	int dest = getProcessIdByCell(ni, nj);
  	cell* C = new cell();
  	C->h = i;
  	C->w = j;
  	C->value = value;
  	if (dest != id)
  	{
	  	sendRequests.push(MPI::COMM_WORLD.Isend(C, 1, cell_type, dest, 0));
  	}
  }

  void recvCell(int i, int j)
  {
  	normalize(i, j);
  	int source = getProcessIdByCell(i, j);
  	std::pair<MPI::Request, cell*> request;
  	request.second = new cell();
  	if (source != id)
  	{
	  	request.first = MPI::COMM_WORLD.Irecv(request.second, 1, cell_type, source, 0);
	  	recvRequests.push(request);
  	}
  }

	int cellId(int h, int w) const
	{
		if (h - (uH - 1) < 0)
		{
			h += N;
		}
		if (h > dH)
		{
			h -= N;
		}	
		if (w  - (lW - 1) < 0)
		{
			w += N;
		}
		if (w > rW)
		{
			w -= N;
		}
		return (h - (uH - 1)) * (wSize + 1) + (w - (lW - 1));
	}

	void getPosById(int id, int& wPos, int& hPos) const
	{
		hPos = (id - 1) / wChunks;
		wPos = (id - 1) % wChunks;
	}

	int getProcessIdByCell(int w, int h) const
	{
		int wPos = w / wSize;
		int hPos = h / hSize;
		return idByPos(wPos, hPos);
	}

	int idByPos(int wPos, int hPos) const
	{
		return hPos * wChunks + wPos + 1;
	}

	int id;
	int uH, dH, lW, rW;
	int N, wChunks, hChunks;
	int wSize, hSize;
	int myWSize, myHSize;
	char* grid;
	char* buf;
	std::queue<MPI::Request> requests;
	std::queue<MPI::Request> sendRequests;
	std::queue<std::pair<MPI::Request, cell*>> recvRequests;
};

int main(int argc, char* argv[]) {
  int id, num_procs;

  MPI::Init(argc, argv);
  id = MPI::COMM_WORLD.Get_rank();
  num_procs = MPI::COMM_WORLD.Get_size();

  cell_type = MPI::INT.Create_contiguous(3);
  cell_type.Commit();

  int wChunks = sqrt(num_procs - 1);
  int hChunks = (num_procs - 1) / wChunks;
  int working_procs = wChunks * hChunks;

  if (id == 0) 
  {
	  cerr << "Working processes " << working_procs << endl;
		cerr << "Hello, I am master process " << id << endl;
	  if (argc != 5) 
	  {
	    fprintf(stderr, "Usage: %s N input_file iterations output_file\n", argv[0]);
	    return 1;
	  }

	  int N = atoi(argv[1]); // grid size
	  int iterations = atoi(argv[3]);

	  FILE* input = fopen(argv[2], "r");
	  char* grid = (char*) malloc(N * N * sizeof(char));
	  for (int i = 0; i < N; ++i) 
	  {
	    fscanf(input, "%s", grid + i * N);
	  }
	  fclose(input);

	  cerr << id << ": " << "Successfully finished reading input" << endl;

  	for (int proc = 1; proc <= working_procs; ++proc)
  	{
	  	MPI::COMM_WORLD.Send(&iterations, 1, MPI::INT, proc, 0);
	  	MPI::COMM_WORLD.Send(&N, 1, MPI::INT, proc, 0);
  	}

  	for (int proc = 1; proc <= working_procs; ++proc) 
  	{
			int wSize = (N + wChunks - 1) / wChunks;
			int hSize = (N + hChunks - 1) / hChunks;
			int hPos = (proc - 1) / wChunks;
			int wPos = (proc - 1) % wChunks;
			int uH = hPos * hSize;
			int dH = std::min(uH + hSize, N);
			int lW = wPos * wSize;
			int rW = std::min(lW + wSize, N);
			hSize = dH - uH;
			wSize = rW - lW;
			
			std::queue<MPI::Request> requests;
			int totalSent = 0;
			for (int h = uH - 1; h < dH + 1; ++h)
			{
				for (int w = lW - 1; w < rW + 1; ++w)
				{
					++totalSent;
					requests.push(MPI::COMM_WORLD.Isend(grid + toindex(h, w, N), 1, MPI::UNSIGNED_CHAR, proc, 0));
				}
			}
			while (!requests.empty())
			{
				requests.front().Wait();
				requests.pop();
			}
			cerr << id << ": " << "Grid to " << proc << " was sent" << endl;
			cerr << id << ": " << "Total sent: " << totalSent << endl;
  	}

	  // Gather everything here
	  // FILE* output = fopen(argv[4], "w");
	  // printgrid(grid, output, N);
	  // fclose(output);

	  // free(grid);
  }
  else
  if (id <= working_procs)
  {
  	cerr << "Hello, I am process " << id << endl;
  	int iterations;
  	int N;
		MPI::COMM_WORLD.Recv(&iterations, 1, MPI::INT, 0, 0);
		cerr << id << ": " << "Received iterations = " << iterations << endl;
		MPI::COMM_WORLD.Recv(&N, 1, MPI::INT, 0, 0);
		cerr << id << ": " << "Received N = " << N << endl;

  	ProcessWorkInfo processWorkInfo(id, N, wChunks, hChunks);
	  for (int iter = 0; iter < iterations; ++iter) {
	  	processWorkInfo.updateGrid();
	  	cerr << id << ": " << "Iteration " << iter << " finished" << endl;
	  }
	  // processWorkInfo.sendBack();
  }
  else
  {
  	return 0;
  }

  MPI::Finalize();

  return 0;
}