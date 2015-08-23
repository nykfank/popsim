typedef struct {
	short x,y;
} s_path;

typedef struct {
	int total,follow,frame,num_activated;
	int inactive,moving,blocked,waiting,offpath,finished;
	int *activated;
	char *start_name,*start_adr,*current_adr;
	float zoomfakt,spritezoom;
	s_path view;
} s_sim;

typedef struct {
	short path_pos,wait_count,node1,node2,target_node;
	s_path startpos,pos,nodepos;
	char *name;
	bool on_path,active,finished;
} s_person;

void file_error_check(char *filename,FILE *file_pointer) {
	if (file_pointer==NULL) {
		perror(filename);
		exit(1);
	}
}

int path_file_length(char* filename) {
	FILE *file_pointer;
	file_pointer=fopen(filename,"r");
	file_error_check(filename,file_pointer);
	int i=0;
	char *buf;
	buf = malloc(MAX_LINE_LEN*sizeof(char)); // free OK
	while (fgets (buf,MAX_LINE_LEN,file_pointer)!=NULL) {i++;}
	free(buf);
	fclose(file_pointer);
	return i;
}

s_path* load_path(char* filename) {
	s_path *pfad;
	FILE *file_pointer;
	char *buf,*ptr;
	buf = malloc(MAX_LINE_LEN*sizeof(char)); // free OK
	file_pointer=fopen(filename,"r");
	file_error_check(filename,file_pointer);
	pfad=malloc(sizeof(s_path)*(path_file_length(filename)+1)); // free OK
	int i=0;
	while (fgets (buf,MAX_LINE_LEN,file_pointer)!=NULL) {
		buf[strlen(buf)-1] = '\0';
		ptr = strtok(buf, " ");
		pfad[i].x=atoi(ptr);
		ptr = strtok(NULL, " ");
		pfad[i].y=atoi(ptr);
		i++;
	}
	fclose(file_pointer);
	pfad[i].x=-1;
	pfad[i].y=-1;
	free(buf);
	return pfad;
}

int path_length(s_path *p) {
	int j=0;
	while (1) {
		if (p[j].x==-1) {break;}
		j++;
	}
	return j;
}

void mark_map(s_path pos,gdImagePtr pin,unsigned char **usemap,gdImagePtr img) {
	int c;
	s_path i;
	for (i.x=pin->sx/-2;i.x<pin->sx/2;i.x++) {
		for (i.y=pin->sy/-2;i.y<pin->sy/2;i.y++) {
			if ((pos.x+i.x<0) || (pos.x+i.x>img->sx)) {continue;}
			if ((pos.y+i.y<0) || (pos.y+i.y>img->sy)) {continue;}
			c=gdImageGetPixel(pin,i.x+pin->sx/2,i.y+pin->sy/2);
			if (gdTrueColorGetRed(c)>0) {
				usemap[pos.x+i.x][pos.y+i.y]=gdTrueColorGetRed(c);
			}
		}
	}
}

void unmark_map(s_path pos,gdImagePtr pin,unsigned char **usemap,gdImagePtr img) {
	int c;
	s_path i;	
	for (i.x=pin->sx/-2;i.x<pin->sx/2;i.x++) {
		for (i.y=pin->sy/-2;i.y<pin->sy/2;i.y++) {
			if ((pos.x+i.x<0) || (pos.x+i.x>img->sx)) {continue;}
			if ((pos.y+i.y<0) || (pos.y+i.y>img->sy)) {continue;}
			c=gdImageGetPixel(pin,i.x+pin->sx/2,i.y+pin->sy/2);
			if (gdTrueColorGetRed(c)==usemap[pos.x+i.x][pos.y+i.y]) {
				usemap[pos.x+i.x][pos.y+i.y]=0;
			}
		}
	}
}

gdImagePtr create_frame(gdImagePtr img,unsigned char **usemap,s_sim sim) {
	gdImagePtr oimg;
	int c;
	s_path pos;
	oimg=gdImageCreateTrueColor(OUTSIZE_X*sim.zoomfakt,OUTSIZE_Y*sim.zoomfakt);
	for (pos.x=sim.view.x-OUTSIZE_X/2*sim.zoomfakt;pos.x<sim.view.x+OUTSIZE_X/2*sim.zoomfakt;pos.x++) {
	for (pos.y=sim.view.y-OUTSIZE_Y/2*sim.zoomfakt;pos.y<sim.view.y+OUTSIZE_Y/2*sim.zoomfakt;pos.y++) {
		if ((pos.x<0) || (pos.x>=img->sx)) {continue;}
		if ((pos.y<0) || (pos.y>=img->sy)) {continue;}
		if (usemap[pos.x][pos.y]>0) {c = gdImageColorAllocate(oimg,usemap[pos.x][pos.y],0,0);}
		else {c=gdImageGetPixel(img,pos.x,pos.y);}
		gdImageSetPixel(oimg,pos.x-sim.view.x+OUTSIZE_X/2*sim.zoomfakt,
		pos.y-sim.view.y+OUTSIZE_Y/2*sim.zoomfakt,c);
	}
	}
	return oimg;
}

bool scan_region(s_path pos,unsigned char **usemap,gdImagePtr img) {
	s_path i;
	for (i.x=-ACTIVATION_DISTANCE/2;i.x<ACTIVATION_DISTANCE/2;i.x++) {
		for (i.y=-ACTIVATION_DISTANCE/2;i.y<ACTIVATION_DISTANCE/2;i.y++) {
			if ((pos.x+i.x<0) || (pos.x+i.x>img->sx)) {continue;}
			if ((pos.y+i.y<0) || (pos.y+i.y>img->sy)) {continue;}
			if (usemap[pos.x+i.x][pos.y+i.y]==COLLISION_INDEX_RED) {return true;}
		}
	}
	return false;
}

void save_positions(s_person *person,s_sim sim) {
	int i;
	char *filename;
	FILE *file_pointer;
	filename=malloc(sizeof(char)*100); // free OK
	sprintf(filename,"frame%06d.pos",sim.frame);
	file_pointer=fopen(filename,"wb");
	file_error_check(filename,file_pointer);
	for (i=0;i<sim.total;i++) {
		if (!person[i].active) {continue;}
		if (person[i].wait_count!=0) {continue;}
		fwrite(&person[i].pos, sizeof(person[i].pos), 1, file_pointer);
	}
	fclose(file_pointer);
	free(filename);	
}

void save_identifiers(s_person *person,s_sim sim) {
	int i;
	char *filename;
	FILE *file_pointer;
	filename=malloc(sizeof(char)*100); // free OK
	sprintf(filename,"frame%06d.ids",sim.frame);
	file_pointer=fopen(filename,"wb");
	file_error_check(filename,file_pointer);
	for (i=0;i<sim.total;i++) {
		if (!person[i].active) {continue;}
		if (person[i].wait_count!=0) {continue;}
		fwrite(&i, sizeof(i), 1, file_pointer);
	}
	fclose(file_pointer);
	free(filename);
}

void save_frame(gdImagePtr frame,s_sim sim) {
	char *filename;
	FILE *file_pointer;
	filename=malloc(sizeof(char)*100); // free OK
	sprintf(filename,"frame%06d.jpg",sim.frame);
	file_pointer = fopen(filename, "wb");
	file_error_check(filename,file_pointer);
	gdImageJpeg(frame, file_pointer,JPEG_QUALITY);
	fclose(file_pointer);
	free(filename);
}

