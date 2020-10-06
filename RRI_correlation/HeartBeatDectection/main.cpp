#define _CRT_SECURE_NO_WARNINGS
#include <tchar.h>
#include <stdio.h>
#include <iostream>
#define _USE_MATH_DEFINES
#include <math.h>
#include <time.h>

struct tm global_time;

int ReadFile_ID = 6;
double *tdata; //�v�����ꂽ���U���Ԃ̃f�[�^
int *sdata[2];	//�Z���T�f�[�^
int tstep_cnt;  //���n��f�[�^�̃X�e�b�v��

//------------------------------------------------------
double rnd() //0�ȏ�1����
{
	return((double)(rand() % RAND_MAX) / RAND_MAX);
}

//------------------------------------------------------
double rnd2() //0�ȏ�1�ȉ�
{
	return((double)((rand() % (RAND_MAX + 1)) / RAND_MAX));
}

//------------------------------------------------------
double rndn()
{
	return (rnd() + rnd() + rnd() + rnd() + rnd() + rnd() +
		rnd() + rnd() + rnd() + rnd() + rnd() + rnd() - 6.0);
}


//------------------------------------------------------
void release_alloc()
{
	int i;

	for (i = 0; i < 2; i++) {
		free(sdata[i]);
	}
	free(tdata);
}

//------------------------------------------------------
void init() 
{
	int i, j;
	FILE *fp;
	const int buf_size = 100;
	char buf[buf_size];
	char rfilename[30] = { 0 };
	int row = 0, col = 3; //�s�Ɨ�̐�

	sprintf(rfilename, "data%d.csv", ReadFile_ID);

	// �f�[�^���̃J�E���g
	fp = fopen(rfilename, "r");
	if (fp) {
		while (fgets(buf, buf_size, fp) != NULL) {
			row++;
			memset(buf, 0, sizeof(buf));
		}
		printf("Row= %d , Column = %d \n", row, col);
	}
	tstep_cnt = row;
	
	// ���I�������̊m��
	tdata = (double*)malloc(sizeof(double)*tstep_cnt);
	for(i = 0; i < 2; i++)
		sdata[i] = (int*)malloc(sizeof(int)*tstep_cnt);

	// ���I�������Ŋm�ۂ����ϐ��Ƀf�[�^����
	fp = fopen(rfilename, "r");
	if (fp) {
		for (i = 0; i < tstep_cnt; i++) {
			for (j = 0; j < col - 1; j++) {
				if(j == 0) fscanf(fp, "%lf,", &tdata[i]);
				else fscanf(fp, "%d\n", &sdata[0][i]); //ECG
			}
			//fscanf(fp, "%d\n", &sdata[1][i]); //BCG

			//printf("%.2lf, %d, %d\n", tdata[i], sdata[0][i], sdata[1][i]);
		}

		fclose(fp);
	}
}

//--------------------------------------------------
void ECG_HearBeatDect() 
{
	int i, j, k, t = 0, s = 1;
	int ini = 50, max_steps = 100, max_data = 0; // ini: ���ւ̊m�F�̊J�n�X�e�b�v�Cmax_steps: �̈�̕�
	double ar = 0, ave = 0,max_ar;
	double hb_time[2] = {0}; // 0: ����, 1: 1�X�e�b�v�O 
	double alfa = 1.0;
	int hb_time_memory;
	int year, month, date, hour, min, sec; 

	FILE *fp;
	char wfilename[50] = { 0 };

	//--------------�����o���t�@�C�������ԂŃl�[�~���O���邽��----------------
	time_t tt = time(NULL);
	localtime_s(&global_time, &tt);
	year = global_time.tm_year + 1900;
	month = global_time.tm_mon + 1;
	date = global_time.tm_yday + 1;
	hour = global_time.tm_hour;
	min = global_time.tm_min;
	sec = global_time.tm_sec;
	//-----------------------------------------------------------------------

	// RRI�̃t�@�C�������o��
	sprintf(wfilename, "RRI_data%d_%d%d%d%d%d-%d%d%d%d%d%d.csv", ReadFile_ID, 
		year, (int)(month / 10), (int)(month % 10), (int)(date / 10), (int)(date % 10),
		(int)(hour / 10), (int)(hour % 10), (int)(min / 10), (int)(min % 10), (int)(sec / 10), (int)(sec % 10));
	
	//----------------------------------------------------------------------

	fp = fopen(wfilename, "w");

	while (s) {
		for (i = ini; i < ini + max_steps; i++) {
			if ((t + i * 2) >= tstep_cnt) { s = 0; break; }
			ave = 0;
			ar = 0;
			for (j = t; j < i * 2 + t; j++) ave += (1.0 / (double)(i * 2)) * (double)sdata[0][j]; //���ϒl
			for (j = t; j < i + t; j++) ar += (1.0 / (double)i) * (((double)sdata[0][j] - ave) * ((double)sdata[0][j + i] - ave));

			if (i == ini) {
				k = i;
				max_ar = ar;
			}
			else {
				if (max_ar < ar) {
					k = i;
					max_ar = ar; //���֍ő�
				}
			}
		}

		alfa = 1.0;
		if (t > 0) alfa = 1.25; //�T���̈�𐄒肳��������1.25�{�Ɋg�����ĒT���i����̒T���͏����j
		max_data = -100; j = t; //���ȑ��ւ��瑊�֍ő�ƂȂ�������킩��̂ŁC�g�`�̂��͈̔͂ōő�l��T�����CR�g�Ƃ���D
		for (i = t; i < k * alfa + t; i++) {
			if (max_data < sdata[0][i]) {
				double p = 1.0;
				if(t > 1) p = exp(-(((i - t) - hb_time_memory) * ((i - t) - hb_time_memory)) / pow((double)k * 0.5, 2));
				if (p > 0.5) {
					max_data = sdata[0][i];
					j = i;
				}
			}
		}
		hb_time_memory = j - t; //�ꎞ�I��R�g�̈ʒu���L��
		hb_time[1] = hb_time[0];
		hb_time[0] = tdata[j];
		//printf("%d, %d, %d, %.2lf, %.3lf\n", j, hb_time_memory, k, max_ar, hb_time[0] - hb_time[1]);
		printf("%d, %.3lf\n", j, hb_time[0] - hb_time[1]);
		if (fp) fprintf(fp, "%lf, %lf\n", hb_time[0], hb_time[0] - hb_time[1]);
		
		if (s == 0) { //���n��f�[�^�I�Ղ̒T��
			max_data = -100; j = t + k; //���ȑ��ւ��瑊�֍ő�ƂȂ�������킩��̂ŁC�g�`�̂��͈̔͂ōő�l��T�����CR�g�Ƃ���D
			for (i = t + k; i < 2 * k + t; i++) {
				if (max_data < sdata[0][i]) {
					double p;
					p = exp(-(((i - t - k) - hb_time_memory) * ((i - t - k) - hb_time_memory)) / pow((double)k * 0.5, 2));
					if (p > 0.5) {
						max_data = sdata[0][i];
						j = i;
					}
				}
			}

			hb_time[1] = hb_time[0];
			hb_time[0] = tdata[j];
			printf("%d, %.3lf\n", j, hb_time[0] - hb_time[1]);
			if (fp) { fprintf(fp, "%lf, %lf\n", hb_time[0], hb_time[0] - hb_time[1]); fclose(fp); }
		}

		max_steps = k;
		ini = (double)k * 0.5;
		t += k;
	}
}

//--------------------------------------------------
void main()
{
	init();
	ECG_HearBeatDect();
	release_alloc();
}