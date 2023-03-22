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
    FILE* csv_file = fopen("sample_raw_frame.csv", "r"); // open file for reading
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

void trunc_rle() {	
  int iter = 0;
    uint8_t *buf = buff;
    memcpy(tx_buff, &PREAMBLE, sizeof(PREAMBLE));
	iter += sizeof(PREAMBLE);
    int length = IMG_ROWS*IMG_COLS;

	// truncate and RLE
	for (int i = 0; i < length; i++){
        // for lane finding
        if (i == LINE_START_ROW*IMG_COLS){line_row1_i = iter;}
        else if (i == LINE_END_ROW*IMG_COLS){line_row2_i = iter;}

    int count = 1;
		while(1){ // find repeating
			if (i < length-1 && (buf[i]&0xF0) == (buf[i+1]&0xF0)){
			  count++;
			  i++;
      }
      else { break; }
		}

		while(count > 0xF){ // count > 15, need another byte
			tx_buff[iter++] = (buf[i]&0xF0) + 0xF;
			count -= 0xF;
		}
 		tx_buff[iter++] = (buf[i]&0xF0) + count;
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

/*
// find 2 black regions (lanes) and return middle pixel for each region
// return 0 if 1 or both not found
bool find_black_pixel(uint8_t * tx_buff, 
        int* i, // starting index into tx_buff
        int* pixel, // starting pixel
        int row_num, // current row number
        int* lcol, int* rcol){
    *lcol=0, *rcol=0;
    int old_pixel=*pixel;
    int black_count=((tx_buff[*i]&0xF0)==0x8)? 0:*pixel%IMG_COLS;
    *pixel+=tx_buff[(*i)++] & 0x0F; // skip the first rle in the row
    while (*pixel-old_pixel<IMG_COLS){
        if ((tx_buff[*i]&0xF0) == 0x0){ // black
            if (*lcol==0){ //leftmost black part
                int count = tx_buff[*i]&0x0F;
                while(*pixel-old_pixel<IMG_COLS &&
                      (tx_buff[*i]&0xF0) == 0x0 &&
                      (tx_buff[*i]&0x0F) == 0xF){
                    count+=tx_buff[*i]&0x0F;
                    *pixel+=tx_buff[(*i)++] & 0x0F;
                }
            *lcol=(*pixel)%row_num + (count)/2; // middle
            }
            else{ //rightmost
                int count = tx_buff[*i]&0x0F;
                while((tx_buff[*i]&0xF0) == 0x0 &&
                      (tx_buff[*i]&0x0F) == 0xF){
                    count+=tx_buff[*i]&0x0F;
                    *pixel+=tx_buff[(*i)++] & 0x0F;
                }
                *lcol=(*pixel)%row_num + (tx_buff[*i]&0x0F)/2; // middle
            }
            black_count+=tx_buff[*i]&0x0F;
        }
        printf("%d %d\n", tx_buff[*i]&0xF0, tx_buff[*i]&0x0F);
        *pixel+=tx_buff[(*i)++] & 0x0F;
    }
    if (black_count >= 140){return false;}
    return true;
}
*/

bool find_lanes(){
    int i, col;
    int lcol1=0, lcol2=0, rcol1=0, rcol2=0;
    int black_count=0;
    bool no_lane=false;
    if (line_row1_i == -1 || line_row2_i == -1){
        printf("dunno what index line should be\n");
        return 0;
    }
    printf("row1i=%d,row2i=%d\n",line_row1_i,line_row2_i);

    // bottom points
    i=line_row1_i, col = 0;
    // TODO: if first col were black
    while (col<IMG_COLS){
        if ((tx_buff[i]&0xF0) == 0x0){ // black
            int count = 0;
            if (lcol1==0){ //leftmost
                int start_col = col;
                count=0;
                while(col+15<IMG_COLS &&
                      (tx_buff[i+1]&0xF0) == 0x0){
                    printf("%d %d\n", tx_buff[i]&0xF0, tx_buff[i]&0x0F);
                    count+=tx_buff[i]&0x0F;
                    col+=tx_buff[i++]&0x0F; // 0xF
                }
                count+=tx_buff[i]&0x0F;
                lcol1=start_col + (count)/2; // middle
            }
            else{ //rightmost
                int start_col = col;
                count=0;
                while(col+15<IMG_COLS &&
                      (tx_buff[i+1]&0xF0) == 0x0){
                    printf("%d %d\n", tx_buff[i]&0xF0, tx_buff[i]&0x0F);
                    count+=tx_buff[i]&0x0F;
                    col+=tx_buff[i++]&0x0F; // 0xF
                }
                count+=tx_buff[i]&0x0F;
                rcol1=start_col + (count)/2; // middle
            }
            black_count+=count;
        }
        printf("%d %d\n", tx_buff[i]&0xF0, tx_buff[i]&0x0F);
        col+=tx_buff[i++]&0x0F;
    }
    printf("black_count=%d\n",black_count);
    if (black_count > 140)
    {
        printf("No lanes found\n");
        return 0;
    }
    black_count=0;

    // top points
    i=line_row2_i, col = 0;
    // TODO: if first col were black
    while (col<IMG_COLS){
        if ((tx_buff[i]&0xF0) == 0x0){ // black
            int count = 0;
            if (lcol2==0){ //leftmost
                int start_col = col;
                count=0;
                while(col+15<IMG_COLS &&
                      (tx_buff[i+1]&0xF0) == 0x0){
                    printf("%d %d\n", tx_buff[i]&0xF0, tx_buff[i]&0x0F);
                    count+=tx_buff[i]&0x0F;
                    col+=tx_buff[i++]&0x0F; // 0xF
                }
                count+=tx_buff[i]&0x0F;
            lcol2=start_col + (count)/2; // middle
            }
            else{ //rightmost
                int start_col = col;
                count=0;
                while(col+15<IMG_COLS &&
                      (tx_buff[i+1]&0xF0) == 0x0){
                    printf("%d %d\n", tx_buff[i]&0xF0, tx_buff[i]&0x0F);
                    count+=tx_buff[i]&0x0F;
                    col+=tx_buff[i++]&0x0F; // 0xF
                }
                count+=tx_buff[i]&0x0F;
                rcol2=start_col + (count)/2; // middle
            }
            black_count+=count;
        }
        printf("%d %d\n", tx_buff[i]&0xF0, tx_buff[i]&0x0F);
        col+=tx_buff[i++]&0x0F;
    }
    printf("black_count=%d\n",black_count);

    // if more than 80% black, no lanes
    if (black_count > 140)
    {
        printf("No lanes found\n");
        return 0;
    }
    else{
        printf("line1=(%d,%d), (%d,%d)\n",LINE_START_ROW,lcol1,LINE_END_ROW,lcol2);
        printf("line2=(%d,%d), (%d,%d)\n",LINE_START_ROW,rcol1,LINE_END_ROW,rcol2);
        return 1;
    }
}
// TODO: black wrap around

int main(){
    read_file();
    rotate_90cw(); // take out after fix hw issue
    trunc_rle();
    find_lanes();
    write_file();
}