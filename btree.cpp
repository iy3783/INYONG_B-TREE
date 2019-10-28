#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <math.h>
#include <vector>
#include <algorithm>
#include <string.h>

using namespace std;



fstream file;
ofstream outputfile;
int blocksize;
int rootBID;
int depth;
int blocknum;
int lastBID=0;
vector<int> v0;
vector<int> v1;

class BTree {
public:
	BTree(const char *fileName);
	void insert(int key, int rid);
	void print();
	void search(int key); // point search
	void rangesearch(int startRange, int endRange); // range search


};

struct splitinfo {
	int BID;
	int key;
};

BTree::BTree(const char *fileName) {
	file.open(fileName, ios::out | ios::in | ios::binary);
	file.seekg(0);
	file.read(reinterpret_cast<char*>(&blocksize), sizeof(int));
	file.seekg(4);
	file.read(reinterpret_cast<char*>(&rootBID), sizeof(int));
	file.seekg(8);
	file.read(reinterpret_cast<char*>(&depth), sizeof(int));

	blocknum = (blocksize - 4)/8;

}

void findlastblock(int BID, int nowdepth) {
	if (depth == nowdepth) {
		int curBID_offset = 12 + ((BID - 1)*blocksize);
		if (BID > lastBID) {
			lastBID = BID;
		}
	}
	else {
		int curBID_offset = 12 + ((BID - 1)*blocksize);
		int i;
		int key;
		int nextBID;
		if (BID > lastBID) {
			lastBID = BID;
		}

		file.seekg(curBID_offset);
		file.read(reinterpret_cast<char*>(&nextBID), sizeof(int));
		if (nextBID!=0) {
			findlastblock(nextBID, nowdepth + 1);

		}
	

		for (i = 0; i < blocknum; i++) {
			file.seekg(curBID_offset + 8 * i + 8);
			file.read(reinterpret_cast<char*>(&nextBID), sizeof(int));
			if (nextBID != 0) {
				findlastblock(nextBID, nowdepth + 1);

			}

		}
	}
	return;

}


splitinfo inserting(int BID, int now_depth,int key, int rid) {
	if (now_depth == depth ) { //leaf node 
		int curBID_offset =12+((BID-1)*blocksize);
		int i;
		for (i = 0; i < blocknum;i++) {
			file.seekg(curBID_offset+ 8 * i);
			int val;
			file.read(reinterpret_cast<char*>(&val), sizeof(int));
			if (val != 0) {
				if (val > key) {
					int tmpkey = val;
					int tmpid;
					file.seekg(curBID_offset + 8 * i + 4);
					file.read(reinterpret_cast<char*>(&tmpid), sizeof(int));
					file.seekp(curBID_offset + 8 * i);
					file.write(reinterpret_cast<char*>(&key), sizeof(int));
					file.seekp(curBID_offset + 8 * i + 4);
					file.write(reinterpret_cast<char*>(&rid), sizeof(int));
					key = tmpkey;
					rid = tmpid;

				}
			}
			if (val == 0) {
				file.seekp(curBID_offset + 8 * i);
				file.write(reinterpret_cast<char*>(&key), sizeof(int));
				file.seekp(curBID_offset + 8 * i +4);
				file.write(reinterpret_cast<char*>(&rid), sizeof(int));
				break;
			}

		}

		if (i == blocknum) {
			
			int endoffset =12+ ((lastBID)*blocksize);
			lastBID = lastBID + 1;
			int t = 0;
			int kk = 0;
			for (int i = 0; i < blocknum; i++) {

				file.seekp(endoffset + i * 8);
				file.write(reinterpret_cast<char*>(&t), sizeof(int));
				file.seekp(endoffset + i * 8 + 4);
				file.write(reinterpret_cast<char*>(&kk), sizeof(int));

			}
			file.seekp(endoffset + blocknum * 8);
			file.write(reinterpret_cast<char*>(&t), sizeof(int));



			int splitedBID = (endoffset - 12) / blocksize + 1;
			int k = 0;
			for (i = (blocknum) / 2; i < blocknum; i++) {
				int tmpkey;
				int tmpval;
				file.seekg(curBID_offset + 8 * i);
				file.read(reinterpret_cast<char*>(&tmpkey), sizeof(int));
				file.seekg(curBID_offset + 8 * i +4 ); 
				file.read(reinterpret_cast<char*>(&tmpval), sizeof(int));
				file.seekp(endoffset + 8*k);
				file.write(reinterpret_cast<char*>(&tmpkey), sizeof(int));
				file.seekp(endoffset + 8 * k +4);
				file.write(reinterpret_cast<char*>(&tmpval), sizeof(int));
				file.seekp(curBID_offset + 8 * i);
				file.write(reinterpret_cast<char*>(&t), sizeof(int));
				file.seekp(curBID_offset + 8 * i + 4);
				file.write(reinterpret_cast<char*>(&kk), sizeof(int));


				k++;

			}//리프노드를 스플릿
			file.seekp(endoffset + 8 * k);
			file.write(reinterpret_cast<char*>(&key), sizeof(int));
			file.seekp(endoffset + 8 * k + 4);
			file.write(reinterpret_cast<char*>(&rid), sizeof(int));
			//마지막 key값 넣기
			int nextleafBID;
			file.seekg(curBID_offset + 8 * blocknum);
			file.read(reinterpret_cast<char*>(&nextleafBID), sizeof(int));
			file.seekp(curBID_offset + 8 * blocknum);
			file.write(reinterpret_cast<char*>(&splitedBID), sizeof(int));
			file.seekp(endoffset + 8 * blocknum);
			file.write(reinterpret_cast<char*>(&nextleafBID), sizeof(int));

			splitinfo si;
			si.BID = splitedBID;
			file.seekg(endoffset);
			file.read(reinterpret_cast<char*>(&si.key), sizeof(int));
			return si;
		}
		

	}
	else {// nonleaf 노드 
		int curBID_offset = 12 + ((BID - 1)*blocksize);
		int i;
		int islast = 1;//맨왼쪽인지
		int will_be_splited=0;//만약 밑에서 스플릿 일어났을때 현재에서 또 스플릿 일어날지 파악
		splitinfo si;
		int splitedpointer;

		for (i = blocknum-1; i>=0; i--) {
			int val;
			file.seekg(curBID_offset + 8 * i+4);
			file.read(reinterpret_cast<char*>(&val), sizeof(int));
			if (i == blocknum - 1) {//만약 꽉차있는 nonleaf라면 split 연쇄 가능
				if (val!=0) {
					will_be_splited = 1;

				}
			}
			if (key > val &&val!=0  ) {
				int nextBID;
				file.seekg(curBID_offset + 8 * i + 8);
				file.read(reinterpret_cast<char*>(&nextBID), sizeof(int));
				islast = 0;//제일 처음 아니라고 check
		
				splitedpointer = i+1;
				si = inserting(nextBID,now_depth+1 ,key,rid);
				
				break;
			}
			
		}
		if (islast == 1) {//처음포인터 위치라면 
			int nextBID;
			file.seekg(curBID_offset);
			file.read(reinterpret_cast<char*>(&nextBID), sizeof(int));
			splitedpointer = 0;
			si = inserting(nextBID, now_depth + 1, key, rid);
		}//재귀적으로 처음 내려가서 리프노드의 위치찾는 작업

		//재귀 호출 끝나고 나와서
		if (si.BID ==0 ) {
			splitinfo sii;
			sii.BID = 0;
			sii.key = 0;
			return sii;
		}
		else {//split 일어나면
			//현재 노드에 추가 하고 새로운 blockID연결

			if (will_be_splited ==1) {
				int swapkey = si.key;
				int swapBID = si.BID;
				for (int i = splitedpointer; i < blocknum; i++) {
					int tmpkey;
					int tmpBID;
					file.seekg(curBID_offset + 8 * i + 4);
					file.read(reinterpret_cast<char*>(&tmpkey), sizeof(int));
					file.seekg(curBID_offset + 8 * i + 8);
					file.read(reinterpret_cast<char*>(&tmpBID), sizeof(int));

					file.seekp(curBID_offset + 8 * i + 4);
					file.write(reinterpret_cast<char*>(&swapkey), sizeof(int));
					file.seekp(curBID_offset + 8 * i + 8);
					file.write(reinterpret_cast<char*>(&swapBID), sizeof(int));

					swapkey = tmpkey;
					swapBID = tmpBID;
				}

				int upvalue;
				int endoffset = 12 + ((lastBID)*blocksize);
				lastBID = lastBID + 1;
			
				int t = 0;
				int kk = 0;
				for (int i = 0; i < blocknum; i++) {

					file.seekp(endoffset + i * 8);
					file.write(reinterpret_cast<char*>(&t), sizeof(int));
					file.seekp(endoffset + i * 8 + 4);
					file.write(reinterpret_cast<char*>(&kk), sizeof(int));

				}
				file.seekp(endoffset + blocknum * 8);
				file.write(reinterpret_cast<char*>(&t), sizeof(int));


				int splitedBID = (endoffset - 12) / blocksize + 1;
				int k = 0;
				for (int i = (blocknum) / 2; i < blocknum; i++) {
					int tmpkey;
					int tmpval;
					if (i== (blocknum) / 2) {
						file.seekg(curBID_offset + 8 * i + 4);
						file.read(reinterpret_cast<char*>(&upvalue), sizeof(int));
						file.seekg(curBID_offset + 8 * i + 8);
						file.read(reinterpret_cast<char*>(&tmpval), sizeof(int));
						file.seekp(endoffset);
						file.write(reinterpret_cast<char*>(&tmpval), sizeof(int));

						file.seekp(curBID_offset + 8 * i + 4);
						file.write(reinterpret_cast<char*>(&t), sizeof(int));
						file.seekp(curBID_offset + 8 * i + 8);
						file.write(reinterpret_cast<char*>(&kk), sizeof(int));

					}
					else {
						file.seekg(curBID_offset + 8 * i + 4);
						file.read(reinterpret_cast<char*>(&tmpkey), sizeof(int));
						file.seekg(curBID_offset + 8 * i + 8);
						file.read(reinterpret_cast<char*>(&tmpval), sizeof(int));
						file.seekp(endoffset + 8 * k + 4);
						file.write(reinterpret_cast<char*>(&tmpkey), sizeof(int));
						file.seekp(endoffset + 8 * k + 8);
						file.write(reinterpret_cast<char*>(&tmpval), sizeof(int));
						file.seekp(curBID_offset + 8 * i + 4);
						file.write(reinterpret_cast<char*>(&t), sizeof(int));
						file.seekp(curBID_offset + 8 * i + 8);
						file.write(reinterpret_cast<char*>(&kk), sizeof(int));

						k++;
					}

				}//논리프노드를 스플릿
				file.seekp(endoffset + 8 * k+4);
				file.write(reinterpret_cast<char*>(&swapkey), sizeof(int));
				file.seekp(endoffset + 8 * k + 8);
				file.write(reinterpret_cast<char*>(&swapBID), sizeof(int));
				
				splitinfo sii;
				sii.BID = splitedBID;
				sii.key = upvalue;
				return sii;
			}
			else {
				int swapkey = si.key;
				int swapBID = si.BID;
				for (int i = splitedpointer; i < blocknum; i++) {
					int tmpkey;
					int tmpBID;
					file.seekg(curBID_offset + 8 * i + 4);
					file.read(reinterpret_cast<char*>(&tmpkey), sizeof(int));
					file.seekg(curBID_offset + 8 * i + 8);
					file.read(reinterpret_cast<char*>(&tmpBID), sizeof(int));
				
					file.seekp(curBID_offset + 8 * i + 4);
					file.write(reinterpret_cast<char*>(&swapkey), sizeof(int));
					file.seekp(curBID_offset + 8 * i + 8);
					file.write(reinterpret_cast<char*>(&swapBID), sizeof(int));

					swapkey = tmpkey;
					swapBID = tmpBID;
				}


				splitinfo sii;
				sii.BID = 0;
				sii.key = 0;
				return sii;

			}
			//만약  현재 노드에서 또 스플릿 일어나면 return 현재 blockID 아니면 return 0
		}
	}

	splitinfo si;
	si.BID = 0;
	si.key = 0;
	return si;
}
void BTree::insert(int key, int rid) {
	splitinfo si;
	if (rootBID == 0) {
		file.seekp(12);
		file.write(reinterpret_cast<char*>(&key), sizeof(int));
		file.seekp(16);
		file.write(reinterpret_cast<char*>(&rid), sizeof(int));
		int t = 0;
		int k = 0;
		for (int i = 1; i < blocknum;i++) {
			
			file.seekp(12+i*8 );
			file.write(reinterpret_cast<char*>(&t), sizeof(int));
			file.seekp(16+i*8);
			file.write(reinterpret_cast<char*>(&k), sizeof(int));

		}
		file.seekp(12 + blocknum * 8);
		file.write(reinterpret_cast<char*>(&t), sizeof(int));
		rootBID = 1;


		return;
	}
	else {
		findlastblock( rootBID,0);
		si = inserting(rootBID, 0, key, rid);
	}
	
	if (si.BID == 0) {//최종루트에서 스플릿 안일어날떄

	}
	else {
		int endoffset = 12 + ((lastBID)*blocksize);
		lastBID = lastBID + 1;
		int t = 0;
		int k = 0;
		for (int i = 0; i < blocknum; i++) {
		
			file.seekp(endoffset + i * 8);
			file.write(reinterpret_cast<char*>(&t), sizeof(int));
			file.seekp(endoffset + i * 8 +4);
			file.write(reinterpret_cast<char*>(&k), sizeof(int));

		}
		file.seekp(endoffset + blocknum * 8);
		file.write(reinterpret_cast<char*>(&t), sizeof(int));


		int splitedBID = (endoffset - 12) / blocksize + 1;
		file.seekp(endoffset);
		file.write(reinterpret_cast<char*>(&rootBID), sizeof(int));
		file.seekp(endoffset + 4);
		file.write(reinterpret_cast<char*>(&si.key), sizeof(int));
		file.seekp(endoffset + 8);
		file.write(reinterpret_cast<char*>(&si.BID), sizeof(int));

		rootBID = splitedBID;
		depth = depth + 1;
		file.seekp(4);
		file.write(reinterpret_cast<char*>(&rootBID), sizeof(int));
		file.seekp(8);
		file.write(reinterpret_cast<char*>(&depth), sizeof(int));


	}


}
void printing(int BID,int nowdepth  ) {
	if (depth < nowdepth) {
		return;
	}
	if (nowdepth == 2) {

		return;
	}
	if (depth == nowdepth) {
		int curBID_offset = 12 + ((BID - 1)*blocksize);
		int i;
		for (i = 0; i < blocknum; i++) {
			int key;
			
			file.seekg(curBID_offset + 8 * i );
			file.read(reinterpret_cast<char*>(&key), sizeof(int));
			if (key != 0) {
				if (nowdepth == 0) {
					v0.push_back(key);

				}
				else if (nowdepth == 1) {
					v1.push_back(key);

				}

			}	
		}
	}
	else {
		int curBID_offset = 12 + ((BID - 1)*blocksize);
		int i;
		int key;
		int nextBID;
		file.seekg(curBID_offset);
		file.read(reinterpret_cast<char*>(&nextBID), sizeof(int));
		if (nextBID!=0) {
			printing(nextBID, nowdepth + 1);
		}
	

		for (i = 0; i < blocknum; i++) {
		
			file.seekg(curBID_offset + 8 * i+4);
			file.read(reinterpret_cast<char*>(&key), sizeof(int));
			if (key != 0) {
				if (nowdepth == 0) {
					v0.push_back(key);
				}
				else if (nowdepth == 1) {
					v1.push_back(key);
				}
			}
			file.seekg(curBID_offset + 8 * i + 8);
			file.read(reinterpret_cast<char*>(&nextBID), sizeof(int));
			if (nextBID != 0) {
				printing(nextBID, nowdepth + 1);
			}
		}
	}
	return;
	
}


void BTree::print() {

	printing(rootBID,0 );
	if (v0.size()!=0) {
		int size = v0.size();
		outputfile << "<0>" << endl;
		if (v0.size() ==1) {
			outputfile << v0[0]<< endl;
		}
		else {
			for (int i = 0; i < size-1; i++) {
				outputfile << v0[i] <<", ";
			}
			outputfile << v0[size-1] << endl;
		}
		
	}
	if (v1.size() != 0) {
		int size = v1.size();
		outputfile << "<1>" << endl;
		if (v1.size()==1) {
			outputfile << v1[0] << endl;
		}
		else {
			for (int i = 0; i < size-1; i++) {
				outputfile << v1[i] << ", ";
			}
			outputfile << v1[size-1] << endl;
		}
		
	}
	return;
}

int searching( int BID,int now_depth,int searchkey) {
	
	if (depth == now_depth) {
		int cur_BID = BID;
	
		while (cur_BID!=0) {
			int curBID_offset = 12 + ((cur_BID - 1)*blocksize);
			for (int i = 0; i < blocknum; i++) {
				int tmpkey;
				file.seekg(curBID_offset + 8 * i);
				file.read(reinterpret_cast<char*>(&tmpkey), sizeof(int));
				if (tmpkey == 0) {

					break;
				}
				if (tmpkey==searchkey) {
					int tmprid;
					file.seekg(curBID_offset + 8 * i+4);
					file.read(reinterpret_cast<char*>(&tmprid), sizeof(int));
					return tmprid;

				}
				else if (tmpkey>searchkey) {
					return 0;

				}

			}
			int nextleafnodeID;
			file.seekg(curBID_offset + 8 *blocknum);
			file.read(reinterpret_cast<char*>(&nextleafnodeID), sizeof(int));
			cur_BID = nextleafnodeID;

		}
		

	
	}
	else {
		int curBID_offset = 12 + ((BID - 1)*blocksize);
		int i;
		int key;
		int nextBID;
	

		file.seekg(curBID_offset);
		file.read(reinterpret_cast<char*>(&nextBID), sizeof(int));
		int a=searching(nextBID, now_depth + 1,searchkey);
		return a;
	}
	return 0;


}


void BTree::search(int key) {

	int a=searching(rootBID, 0, key);
	outputfile << key<<","<< a << endl;


}

void rangesearching(int BID, int now_depth, int start,int end) {

	if (depth == now_depth) {
		int cur_BID = BID;

		while (cur_BID != 0) {
			int curBID_offset = 12 + ((cur_BID - 1)*blocksize);
			for (int i = 0; i < blocknum; i++) {
				int tmpkey;
				file.seekg(curBID_offset + 8 * i);
				file.read(reinterpret_cast<char*>(&tmpkey), sizeof(int));
				if (tmpkey == 0) {

					break;
				}
				if (tmpkey<=end &&tmpkey >=start ) {
					int tmprid;
					file.seekg(curBID_offset + 8 * i+4);
					file.read(reinterpret_cast<char*>(&tmprid), sizeof(int));
					outputfile << tmpkey << "," << tmprid << "\t";

				}
		

			}
			int nextleafnodeID;
			file.seekg(curBID_offset + 8 * blocknum);
			file.read(reinterpret_cast<char*>(&nextleafnodeID), sizeof(int));
			cur_BID = nextleafnodeID;

		}



	}
	else {
		int curBID_offset = 12 + ((BID - 1)*blocksize);
		int i;
		int key;
		int nextBID;


		file.seekg(curBID_offset);
		file.read(reinterpret_cast<char*>(&nextBID), sizeof(int));
		rangesearching(nextBID, now_depth + 1, start,end);
	
	}
	return;


}
void BTree::rangesearch(int startRange, int endRange) {

	rangesearching(rootBID,0,startRange,endRange);
	outputfile<<endl;

}




int main(int argc,char* argv[]) {

	char command = argv[1][0];

	const char *filename;

	
	switch (command)
	{
	case 'c': {
		filename = argv[2];
		blocksize = atoi(argv[3]);
		
		blocksize = 36;

		rootBID = 0;
		depth = 0;

		file.open(filename, ios::out | ios::in | ios::binary | ios::trunc);
		file.seekp(0);
		file.write(reinterpret_cast<char*>(&blocksize), sizeof(int));
		file.seekp(4);
		file.write(reinterpret_cast<char*>(&rootBID), sizeof(int));
		file.seekp(8);
		file.write(reinterpret_cast<char*>(&depth), sizeof(int));
		break;
	}
	case 'i': {
		filename = argv[2];
		const char *inputfilename = argv[3]; //argv 로 b+트리 파일과  insert할 파일 이름 입력받음

	

		BTree *mybtree = new BTree(filename);
		ifstream insertfile;
		insertfile.open(inputfilename,ios::in);
		string S;
		vector<string> V;


		//b+트리 open 하고 insert file open한다.
	

		while(insertfile >>S) {
			V.push_back(S);
		}
		for (int i = 0; i < V.size();i++) { //string vector 입력받아 , 기준으로 또 나눔
			stringstream SSS(V[i]);
			vector<int> V2;
			string tmpstr;
			while (getline(SSS, tmpstr, ',' )) {
				V2.push_back(stoi(tmpstr));
			}
			mybtree->insert(V2[0],V2[1]);
			
		}
	
		file.seekp(4);
		file.write(reinterpret_cast<char*>(&rootBID), sizeof(int));
		file.seekp(8);
		file.write(reinterpret_cast<char*>(&depth), sizeof(int));



		insertfile.close();
		break;
	}
	case 'r': {

		filename = argv[2];
		const char *inputfilename = argv[3];
		
		const char* outputfilename = argv[4];


		BTree *mybtree = new BTree(filename);
		ifstream insertfile;
		insertfile.open(inputfilename, ios::in);
		string S;
		vector<string> V;
		outputfile.open(outputfilename, ios::out | ios::trunc);

		while (insertfile >> S) {
			V.push_back(S);
		}
		for (int i = 0; i < V.size(); i++) { //string vector 입력받아 , 기준으로 또 나눔
			stringstream SSS(V[i]);
			vector<int> V2;
			string tmpstr;
			while (getline(SSS, tmpstr, ',')) {
				V2.push_back(stoi(tmpstr));
			}
			mybtree->rangesearch(V2[0], V2[1]);

		}



		insertfile.close();
		break;
	}
	case 's': {
		filename = argv[2];
		const char *outputfilename = argv[4]; //argv 로 b+트리 파일과  output할 파일 이름 입력받음
		const char *inputfilename= argv[3];
	

	
		ifstream insertfile;
		insertfile.open(inputfilename, ios::in);
		string S;
		vector<string> V;
		BTree *mybtree = new BTree(filename);
		outputfile.open(outputfilename, ios::out | ios::trunc);

		while (insertfile >> S) {
			mybtree->search(stoi(S));
		}
		insertfile.close();
		break;
	}
	case 'p': {
		filename = argv[2];
		const char *outputfilename = argv[3]; //argv 로 b+트리 파일과  output할 파일 이름 입력받음
	
		
		BTree *mybtree = new BTree(filename);
		
		outputfile.open(outputfilename,ios::out|ios::trunc);
		mybtree->print();


		break;
	}

	}


	outputfile.close();
	file.close();
	return 0;
}