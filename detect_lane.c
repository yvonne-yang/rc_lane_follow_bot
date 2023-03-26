#include <math.h>
#include <stdio.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>
#define LINE_START_ROW 135
#define LINE_END_ROW 110

#define PREAMBLE "\r\n!START!\r\n"
#define SUFFIX "!END!\r\n"
#define IMG_ROWS 144
#define IMG_COLS 174
uint8_t buff[IMG_COLS * IMG_ROWS];
uint8_t tx_buff[sizeof(PREAMBLE) + IMG_ROWS * IMG_COLS/2 + sizeof(SUFFIX)]; // trunc

int line_row1_i=-1, line_row2_i=-1; 

void read_file(){
    FILE* csv_file = fopen("0326_straight_half_frame.csv", "r"); // open file for reading
    if (csv_file == NULL) {
        printf("Failed to open file!\n");
        return;
    }

    // Read data from CSV file into array
    int i = 0, a = 9;
    while (fscanf(csv_file, " %d", &a) != EOF && i < IMG_COLS * IMG_ROWS){
        buff[i++] = (uint8_t)a;
    }
    fclose(csv_file); // Close the file

    /*
    for (int i = 0; i < IMG_COLS*IMG_ROWS;i++){
        printf("%d ", buff[i]);
        if (i % IMG_ROWS == 0){
            printf("\n");
        }
    }
    */
}

void write_file(){
    FILE* csv_file = fopen("sample_processed_frame.csv", "w"); 
    if (csv_file == NULL) {
        printf("Failed to open output file!\n");
        return;
    }
    for (int i = 0; i < IMG_ROWS*IMG_COLS; i++) {
        fprintf(csv_file, "%d ", buff[i]);
    }
    fclose(csv_file); // Close the file
    printf("Wrote to file.\n");
}

void rotate_90cw(){
   uint8_t tmp[IMG_ROWS][IMG_COLS];
   for (int j = IMG_ROWS-1; j >= 0; j--){
    for (int i = 0; i < IMG_ROWS; i++){
        tmp[i][IMG_ROWS-1-j] = buff[i+j*IMG_COLS];
    }
   }
   for (int j = IMG_ROWS; j < IMG_COLS; j++){
    for (int i = 0; i < IMG_ROWS; i++){
      tmp[i][j] = -1; //pad with white
    }
   }
   for (int i = 0; i < IMG_ROWS; i++){
    for (int j = 0; j < IMG_COLS; j++){
        buff[i*IMG_COLS+j] = tmp[i][j];
    }
   }
}

#define THRESHOLD 0x70
void trunc_rle() {	
  int iter = 0;
    uint8_t *buf = buff;
    memcpy(tx_buff, &PREAMBLE, sizeof(PREAMBLE));
	iter += sizeof(PREAMBLE);
    int length = IMG_ROWS*IMG_COLS;

	// threshold
	for (int i = 1; i < length; i++){
		buf[i] = ((buf[i]&0xF0)>= THRESHOLD) ? 0x80 : 0;
	}
	
	// truncate and RLE
	for (int i = 0; i < length; i++){
        // for lane finding
        if (i == LINE_START_ROW*IMG_COLS){line_row1_i = iter;}
        else if (i == LINE_END_ROW*IMG_COLS){line_row2_i = iter;}

    int count = 1;
		while(1){ // find repeating
			if (i < length-1 && buf[i] == buf[i+1]){
			  count++;
			  i++;
      }
      else { break; }
		}

		while(count > 0x7F){ // count > 15, need another byte
			tx_buff[iter++] = buf[i]+ 0x7F;
			count -= 0x7F;
		}
 		tx_buff[iter++] = buf[i]+ count;
   }
	
	 /* sanity check
	 int sum = 0;
	 for (int j = sizeof(PREAMBLE); j < iter; j++){
	   sum += tx_buff[j]&0x0F;
	 }
	 char msg[20];
	 sprintf(msg, "%d\n", sum);
	 print_msg(msg);
	 */
}

// find 2 black regions (lanes) and return middle pixel for each region
// return 0 if no lanes found
bool find_black_cols(int i, int* left, int* right){
    *left = -1, *right = -1;
    int col = 0,black_count=0;
    while (col<IMG_COLS){
        if ((tx_buff[i]&0x80) == 0x0){ // black
            int count = 0;
            if (*left==-1){ //leftmost
                int start_col = col;
                count=0;
                while(col+0x7F<IMG_COLS &&
                      (tx_buff[i+1]&0x80) == 0x0){
                    printf("%d %d\n", tx_buff[i]&0x80, tx_buff[i]&0x7F);
                    count+=tx_buff[i]&0x7F;
                    col+=tx_buff[i++]&0x7F; // 0x7F
                }
                if (col+tx_buff[i]&0x7f>=IMG_COLS){
                    count+=IMG_COLS-col;
                }
                else{
                    count+=tx_buff[i]&0x7F;
                }
            *left=start_col + (count)/2; // middle
            }
            else{ //rightmost
                int start_col = col;
                count=0;
                while(col+0x7f<IMG_COLS &&
                      (tx_buff[i+1]&0x80) == 0x0){
                    printf("%d %d\n", tx_buff[i]&0x80, tx_buff[i]&0x7F);
                    count+=tx_buff[i]&0x7F;
                    col+=tx_buff[i++]&0x7F; // 0x7F
                }
                if (col+tx_buff[i]&0x7f>=IMG_COLS){
                    count+=IMG_COLS-col;
                }
                else{
                count+=tx_buff[i]&0x7F;
                }
                *right=start_col + (count)/2; // middle
            }
            black_count+=count;
        }
        printf("%d %d\n", tx_buff[i]&0x80, tx_buff[i]&0x7F);
        col+=tx_buff[i++]&0x7F;
    }
    printf("black_count=%d\n",black_count);
    if (black_count > 100)
    {
        printf("No lanes found\n");
        return 0;
    }
    // if only found one col, determine left or right lane
    if (*right == -1){
        if (*left > IMG_COLS/2){
            *right = *left;
            *left = -1;
        }
    }
    return 1;
}

bool find_lanes(int* botleft, int* botright, int*topleft, int*topright){
    int i, col;
    int black_count=0;
    bool no_lane=false;
    if (line_row1_i == -1 || line_row2_i == -1){
        printf("dunno what index line should be\n");
        return 0;
    }

    // TODO: virtual lines, line extensions
    // TODO: if first col were black
    printf("row i: %d %d\n", line_row1_i, line_row2_i);
    bool ret1 = find_black_cols(line_row1_i,botleft,botright); // bottom
    bool ret2 = find_black_cols(line_row2_i,topleft,topright); // top

    if (*botleft!=-1 && *topleft!=-1){
        printf("left=(%d,%d), (%d,%d)\n",*botleft,LINE_START_ROW,*topleft,LINE_END_ROW);
    }
    if (*botright!=-1 && *topright!=-1){
        printf("right=(%d,%d), (%d,%d)\n",*botright,LINE_START_ROW,*topright,LINE_END_ROW);
    }
    if (!ret1 && !ret2){return 0;}
    else{return 1;}
}
// TODO: black wrap around

bool compute_angles(int* botleft, int* botright, int*topleft, int*topright,
        double* angle){
    if (*botleft == -1 || *topleft==-1){ // only see right lane
        *angle = atan2(*topright-*botright,LINE_START_ROW-LINE_END_ROW)*180/M_PI;
    }
    else if (*botright == -1 || *topright==-1){ // only see left lane
        *angle = atan2(*topleft-*botleft,LINE_START_ROW-LINE_END_ROW)*180/M_PI;
    }
    else {
        float right_ang = atan2(*topright-*botright,LINE_START_ROW-LINE_END_ROW)*180/M_PI;
        printf("right=%f\n",right_ang);
        float left_ang = atan2(*topleft-*botleft,LINE_START_ROW-LINE_END_ROW)*180/M_PI;
        printf("left=%f\n",left_ang);
        *angle = (right_ang + left_ang)/2;
    }
    printf("angle=%f\n",*angle);
    return 1;
}

int main(){
    read_file();
    //rotate_90cw(); // take out after fix hw issue
    trunc_rle();

    int botleft=-1, botright=-1, topleft=-1, topright=-1;
    bool found = find_lanes(&botleft,&botright,&topleft,&topright);
    double angle;
    if (found){
        compute_angles(&botleft,&botright,&topleft,&topright,&angle);
    }

    write_file();
}
