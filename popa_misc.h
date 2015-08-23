typedef struct {
	s_path *path;
	bool exists,alloc,*used;
	int use_count;
} s_edge;

int text_file_length(char* filename) {
	FILE *file_pointer;
	file_pointer=fopen(filename,"r");
	file_error_check(filename,file_pointer);
	int i=0;
	char *buf;
	buf = malloc(MAX_NAME_LEN*sizeof(char)); // free OK
	while (fgets (buf,MAX_NAME_LEN,file_pointer)!=NULL) {i++;}
	free(buf);
	fclose(file_pointer);
	return i;
}

char** load_text(char *filename,int num_lines) {
	char **text,*buf;
	int i=0;
	FILE *file_pointer;
	buf = malloc(MAX_NAME_LEN*sizeof(char)); // free OK
	file_pointer=fopen(filename,"r");
	file_error_check(filename,file_pointer);
	text=malloc(sizeof(char*)*num_lines); // free OK
	while (fgets (buf,MAX_NAME_LEN,file_pointer)!=NULL) {
		text[i]=malloc((strlen(buf)+1)*sizeof(char)); // free OK
		buf[strlen(buf)-1]=0;
		strcpy(text[i],buf);
                i++;

        }
	fclose(file_pointer);
	free(buf);
	return text;
}

int search_person(s_person *p, char *search_key,int total) {
	int i;
	for (i=0;i<total;i++) {
		if (strstr(p[i].name,search_key)) {return i;}
	}
	return 0;
}

short nearest_node(s_path *node_pos,s_path pos) {
	int distance,mindist=INT_MAX;
	short i,minidx;
	s_path pos2;
	for (i=0;i<path_length(node_pos);i++) {
		pos2=node_pos[i];
		distance=(pos.x-pos2.x)*(pos.x-pos2.x)+(pos.y-pos2.y)*(pos.y-pos2.y);
		if (distance<mindist) {
			mindist=distance;
			minidx=i;
		}
	}
	return minidx;
}

bool street_check(s_path pos,gdImagePtr simg) {
	int c;
	c=gdImageGetPixel(simg,pos.x/STREET_SCALE,pos.y/STREET_SCALE);
	if ((gdTrueColorGetRed(c)==255) && (gdTrueColorGetGreen(c)==255) && (gdTrueColorGetBlue(c)==255)) {
		return true;
	}
	return false;
}

s_path* street_path(s_path start,s_path end,gdImagePtr simg) {
 int queue_pos=1,path_pos=1;
 short i,sx,sy;
 s_path **parent,*queue,*path,*path2,node,pos,ii,p1,p2;
 if (start.x<end.x) {p1.x=start.x-REPATH_BORDER;p2.x=end.x+REPATH_BORDER;}
 else {p1.x=end.x-REPATH_BORDER;p2.x=start.x+REPATH_BORDER;}
 if (start.y<end.y) {p1.y=start.y-REPATH_BORDER;p2.y=end.y+REPATH_BORDER;}
 else {p1.y=end.y-REPATH_BORDER;p2.y=start.y+REPATH_BORDER;}
 sx=p2.x-p1.x+1;
 sy=p2.y-p1.y+1;
 queue=malloc(sx*sy*sizeof(s_path)); // free OK
 parent=malloc(sx*sizeof(s_path*)); // bug was here! the missing * after s_path
 for (ii.x=0;ii.x<sx;ii.x++) {
  parent[ii.x]=malloc(sy*sizeof(s_path)); // free OK
  for (ii.y=0;ii.y<sy;ii.y++) {parent[ii.x][ii.y].x=-1;}
 }
 queue[0]=start;
 node=start;
 while (!(node.x==end.x && node.y==end.y) && queue_pos>0) {
  node=queue[0];
  for (i=1;i<queue_pos;i++) {queue[i-1]=queue[i];}
  queue_pos--;
  for (ii.x=-1;ii.x<2;ii.x++) {
   for (ii.y=-1;ii.y<2;ii.y++) {
    if (ii.x==0 && ii.y==0) {continue;}
    pos.x=node.x+ii.x;
    pos.y=node.y+ii.y;
    if (pos.x>=p1.x && pos.x<p2.x && pos.y>=p1.y && pos.y<p2.y) {
     if (street_check(pos,simg)) {
      if (parent[pos.x-p1.x][pos.y-p1.y].x==-1) {
       parent[pos.x-p1.x][pos.y-p1.y]=node;
       queue[queue_pos++]=pos;
      }
     }
    }
   }
  }
 }
 free(queue);
 if (node.x==end.x && node.y==end.y) {
  path=malloc(sx*sy*sizeof(s_path)); // free OK
  pos=end;
  path[0]=end;
  while (!(pos.x==start.x && pos.y==start.y)) {
   pos=parent[pos.x-p1.x][pos.y-p1.y];
   path[path_pos++]=pos;
  }
  path2=malloc(path_pos*sizeof(s_path)); // free OK
  for (i=path_pos-2;i>=0;i--) {
   path2[path_pos-i-2]=path[i];
  }
  path2[path_pos-1].x=-1;
  path2[path_pos-1].y=-1;
  free(path);
 } else {
  path2=malloc(sizeof(s_path)); // free OK
  path2[0].x=-1;
  path2[0].y=-1;
 }
 for (ii.x=0;ii.x<sx;ii.x++) { // fix for memory-leak ?
  free(parent[ii.x]);
 } 
 free(parent);
 return path2;
}

short* node_path(short start,short end,s_edge **edgemap,short sx) {
 int queue_pos=1,path_pos=1;
 short i,node,*queue,pos;
 short *parent,*path,*path2,p1,p2;
 queue=malloc(sx*sizeof(short)); // free OK
 parent=malloc(sx*sizeof(short)); // bug was here! the missing * after s_path
 for (i=0;i<sx;i++) {parent[i]=-1;}
 queue[0]=start;
 node=start;
 while (!(node==end) && queue_pos>0) {
  node=queue[0];
  for (i=1;i<queue_pos;i++) {queue[i-1]=queue[i];}
  queue_pos--;
  for (i=0;i<sx;i++) {
   if (edgemap[node][i].exists==true){
    if (edgemap[i][node].use_count==0){
     if (parent[i]==-1) {
      parent[i]=node;
      queue[queue_pos++]=i;
     }
    }
   }
  }
 }
 free(queue);
 if (node==end) {
  path=malloc(sx*sizeof(short)); // free OK
  pos=end;
  path[0]=end;
  while (pos!=start) {
   pos=parent[pos];
   path[path_pos++]=pos;
  }
  path2=malloc(path_pos*sizeof(short)); // free OK
  for (i=path_pos-2;i>=0;i--) {
   path2[path_pos-i-2]=path[i];
  }
  path2[path_pos-1]=-1;
  free(path);
 } else {
  path2=malloc(sizeof(short)); // free OK
  path2[0]=-1;
 }
 free(parent);
 return path2;
}

s_path directional_move(s_person p) {
	s_path newpos;
	newpos=p.pos;
	if (p.pos.x<p.nodepos.x) {newpos.x++;}
	if (p.pos.y<p.nodepos.y) {newpos.y++;}
	if (p.pos.x>p.nodepos.x) {newpos.x--;}
	if (p.pos.y>p.nodepos.y) {newpos.y--;}
	return newpos;
}

s_edge** check_path(s_edge **edgemap, gdImagePtr simg, s_path *node_pos, s_person p) {
	int pathlen,i;
	if ((p.node1==-1) || (p.node2==-1)) {return edgemap;}
	if (edgemap[p.node1][p.node2].alloc==false) {
		edgemap[p.node1][p.node2].path=street_path(node_pos[p.node1],node_pos[p.node2],simg);
		pathlen=path_length(edgemap[p.node1][p.node2].path);
		if (pathlen==0) {
			edgemap[p.node1][p.node2].exists=false;
			edgemap[p.node2][p.node1].exists=false;
			return edgemap;
		}
		edgemap[p.node1][p.node2].alloc=true;
		edgemap[p.node1][p.node2].used=malloc(sizeof(bool)*pathlen);
		for (i=0;i<pathlen;i++) {edgemap[p.node1][p.node2].used[i]=false;}
	}
	return edgemap;
}

s_path next_pos(s_edge **edgemap, s_person p) {
	s_path pos;
	if ((p.node1==-1) || (p.node2==-1)) {
		pos.x=-1;
		pos.y=-1;
		return pos;
	}
	return edgemap[p.node1][p.node2].path[p.path_pos+1];
}

int person_path_length(s_edge **edgemap, s_person p) {
	if ((p.node1==-1) || (p.node2==-1)) {return 0;}
	if (!edgemap[p.node1][p.node2].exists) {return 0;}
	if (!edgemap[p.node1][p.node2].alloc) {return 0;}
	return path_length(edgemap[p.node1][p.node2].path);
}

int nodepath_length(short *p) {
	int j=0;
	while (1) {
		if (p[j]==-1) {break;}
		j++;
	}
	return j;
}

s_edge** create_edgemap(s_path *node_pos,s_path *edges) {
	s_edge **edgemap;
	int i,j;
	edgemap=malloc(sizeof(s_edge*)*path_length(node_pos));
	for (i=0;i<path_length(node_pos);i++) {
		edgemap[i]=malloc(sizeof(s_edge)*path_length(node_pos));
		for (j=0;j<path_length(node_pos);j++) {
			edgemap[i][j].exists=false;
			edgemap[i][j].alloc=false;
		}
	}
	for (i=0;i<path_length(edges);i++) {
		edgemap[edges[i].x][edges[i].y].use_count=0;
		edgemap[edges[i].x][edges[i].y].exists=true;
		edgemap[edges[i].y][edges[i].x].use_count=0;
		edgemap[edges[i].y][edges[i].x].exists=true;
	}
	return edgemap;
}

s_path* scale_nodes(s_path *node_pos) {
	int i;
	for (i=0;i<path_length(node_pos);i++) {
		node_pos[i].x*=STREET_SCALE;
		node_pos[i].y*=STREET_SCALE;
	}
	return node_pos;
}

s_person next_path(s_path *node_pos,s_edge **edgemap, s_person p) {
	short *nodepath;
	p.node1=p.node2;
	nodepath=node_path(p.node1,p.target_node,edgemap,path_length(node_pos));
	p.node2=nodepath[0];
	p.path_pos=-1;
	free(nodepath);
	return p;
}

s_person previous_path(s_path *node_pos, s_person p) {
	p.node2=p.node1;
	p.node1=-1;
	p.path_pos=-1;
	return p;
}

s_person* create_person_objects(s_sim sim,short *target_nodes,int default_target) {
	s_person *person;
	char **person_names;
	s_path *person_pos;
	int i,nb_targets;
	nb_targets=nodepath_length(target_nodes);
 	person_names=load_text(PERSON_NAME_FILE,sim.total);
	printf("- Loaded %d person names from %s\n",sim.total,PERSON_NAME_FILE);
	person_pos=load_path(PERSON_POS_FILE);
	printf("- Loaded %d person start positions from %s\n",path_length(person_pos),PERSON_POS_FILE);
	person=malloc(sizeof(s_person)*sim.total);
	for (i=0;i<sim.total;i++) {
		person[i].startpos=person_pos[i];
		person[i].pos=person_pos[i];
		person[i].name=malloc((strlen(person_names[i])+1)*sizeof(char)); 
		strcpy(person[i].name,person_names[i]);
		free(person_names[i]);
		person[i].active=false;
		person[i].finished=false;
		person[i].on_path=false;
		person[i].node1=-1;
		person[i].node2=-1;
		person[i].path_pos=-1;
		person[i].wait_count=WAIT_MIN + rand() % WAIT_MAX;
		if (default_target==-1) {person[i].target_node=target_nodes[rand()%nb_targets];} 
		else {person[i].target_node=target_nodes[default_target];}
	}
	free(person_names);
	free(person_pos);
	return person;
}

gdImagePtr load_image(char *filename) {
	gdImagePtr rimg;
	int flen,size;
	FILE *file_pointer;
	flen=strlen(filename);
	file_pointer = fopen(filename, "rb");
	file_error_check(filename,file_pointer);
	fseek(file_pointer , 0L, SEEK_END);
	size = ftell(file_pointer );
	fseek(file_pointer, 0L, SEEK_SET);
	if (size>1000000) {printf("- Loading %i byte from %s\n",size,filename);}
	if (filename[flen-3]=='p') {rimg=gdImageCreateFromPng(file_pointer);}
	else {rimg=gdImageCreateFromJpeg(file_pointer);}
	fclose(file_pointer);
	printf("- Loaded %s (%dx%d)\n",filename,rimg->sx,rimg->sy);
	return rimg;
}

unsigned char** allocate_usemap (gdImagePtr img) {
	unsigned char **usemap;
	int i,j;
	usemap=malloc(sizeof(unsigned char*)*img->sx); // free OK
	for (i=0;i<img->sx;i++) {
		usemap[i]=malloc(sizeof(unsigned char)*img->sy); // free OK
		for (j=0;j<img->sy;j++) {usemap[i][j]=0;}
	}
	return usemap;
}

s_sim parse_address(s_sim sim,char *instr) {
	char *string,*ptr;
	int i;
	string=malloc(sizeof(char)*(strlen(instr)+1));
	strcpy(string,instr);
	ptr=strtok(string, "_");
	sim.start_adr=malloc(sizeof(char)*(strlen(ptr)+1));
	strcpy(sim.start_adr,ptr);
	ptr=strtok(NULL, "_");
	sim.start_name=malloc(sizeof(char)*(strlen(ptr)+1));
	strcpy(sim.start_name,ptr);
	free(string);
	sim.start_name[0]-=32;
	sim.start_adr[0]-=32;
	for (i=0;i<strlen(sim.start_name);i++) {
		if (sim.start_name[i]=='-') {
			sim.start_name[i]=' ';
			if ((sim.start_name[i+1]>96) && (sim.start_name[i+1]<123)) {sim.start_name[i+1]-=32;}
		}
	}
	for (i=0;i<strlen(sim.start_adr);i++) {
		if (sim.start_adr[i]=='-') {
			sim.start_adr[i]=' ';
			if ((sim.start_adr[i+1]>96) && (sim.start_adr[i+1]<123)) {sim.start_adr[i+1]-=32;}
		}
	}
	return sim;
}

gdImagePtr annotate(gdImagePtr oimg2, s_sim sim, s_person *person) {
	char *status;
	s_sim parsed;	
	int brect[8],i,blue,textpos,textpos2;
	blue=gdImageColorAllocate(oimg2, 255, 255, 0);
//	blue=gdImageColorAllocate(oimg2, 50, 50, 250);
	textpos=1;textpos2=0;
	status=malloc(sizeof(char)*500); // free OK
	sprintf(status,"Start: %s",sim.start_adr);
	gdImageStringFT(oimg2,brect,blue,FONT_FILE,FONT_SIZE,0.0,5,FONT_SPACE*textpos++,status);
	sprintf(status,"Position: %s",sim.current_adr);
	gdImageStringFT(oimg2,brect,blue,FONT_FILE,FONT_SIZE,0.0,5,FONT_SPACE*textpos++,status);
	sprintf(status,"Zoom: %2.3f",sim.zoomfakt);
	gdImageStringFT(oimg2,brect,blue,FONT_FILE,FONT_SIZE,0.0,5,FONT_SPACE*textpos++,status);
	sprintf(status,"Total: %d",sim.total);
	gdImageStringFT(oimg2,brect,blue,FONT_FILE,FONT_SIZE,0.0,5,FONT_SPACE*textpos++,status);
	sprintf(status,"Active: %d",sim.total-sim.inactive);
	gdImageStringFT(oimg2,brect,blue,FONT_FILE,FONT_SIZE,0.0,5,FONT_SPACE*textpos++,status);
	sprintf(status,"Waiting: %d",sim.waiting);
	gdImageStringFT(oimg2,brect,blue,FONT_FILE,FONT_SIZE,0.0,5,FONT_SPACE*textpos++,status);
	sprintf(status,"Off path: %d",sim.offpath);
	gdImageStringFT(oimg2,brect,blue,FONT_FILE,FONT_SIZE,0.0,5,FONT_SPACE*textpos++,status);
	sprintf(status,"On path: %d",sim.moving);
	gdImageStringFT(oimg2,brect,blue,FONT_FILE,FONT_SIZE,0.0,5,FONT_SPACE*textpos++,status);
	sprintf(status,"Blocked: %d",sim.blocked);
	gdImageStringFT(oimg2,brect,blue,FONT_FILE,FONT_SIZE,0.0,5,FONT_SPACE*textpos++,status);
	sprintf(status,"Finished: %d",sim.finished);
	gdImageStringFT(oimg2,brect,blue,FONT_FILE,FONT_SIZE,0.0,5,FONT_SPACE*textpos++,status);
	textpos+=7;
	for (i=sim.num_activated-1;i>=0 && i>=sim.num_activated-SHOW_LAST_NUMBER;i--) {
		parsed=parse_address(parsed,person[sim.activated[i]].name);
		sprintf(status," %s",parsed.start_name);
		gdImageStringFT(oimg2,brect,blue,FONT_FILE,FONT_SIZE2,0.0,oimg2->sx-180,
		FONT_SPACE*textpos+FONT_SPACE2*textpos2++,status);
		free(parsed.start_name);
		free(parsed.start_adr);
	}
	free(status);
	return oimg2;
}

char* nearest_address(s_person *person,s_sim sim) {
	int i,distance,minidx,mindist=INT_MAX;
	char *string,*ptr,*address;
	s_path pos;
	s_sim parsed;
	for (i=0;i<sim.total;i++) {
		pos=person[i].startpos;
		distance=(pos.x-sim.view.x)*(pos.x-sim.view.x)+(pos.y-sim.view.y)*(pos.y-sim.view.y);
		if (distance<mindist) {
			mindist=distance;
			minidx=i;
		}
	}
 	parsed=parse_address(parsed,person[minidx].name);
	free(parsed.start_name);
	return parsed.start_adr;
}

bool is_path_used(s_edge **edgemap, s_person p) {
	if ((p.node1==-1) || (p.node2==-1)) {return true;}
	return edgemap[p.node1][p.node2].used[p.path_pos+1];
}

bool does_path_exists(s_edge **edgemap, s_person p) {
	if ((p.node1==-1) || (p.node2==-1)) {return false;}
	return edgemap[p.node1][p.node2].exists;
}

void logg(s_sim sim) {
	printf("frame:%d, off:%d, mov:%d, wt:%d, blck:%d, slp:%d, p:%dx%d, z:%2.3f\n",
	sim.frame,sim.offpath,sim.moving,sim.waiting,sim.blocked,sim.inactive,
	sim.view.x,sim.view.y,sim.zoomfakt);
}

short* find_border_nodes(s_path *node_pos) {
	short i,m,p,*targets,count;
	s_path max,min;
	min.x=SHRT_MAX;
	min.y=SHRT_MAX;
	max.x=0;
	max.y=0;
	for (i=0;i<path_length(node_pos);i++) {
		if (node_pos[i].x>max.x) {max.x=node_pos[i].x;}	
		if (node_pos[i].x<min.x) {min.x=node_pos[i].x;}	
		if (node_pos[i].y>max.y) {max.y=node_pos[i].y;}	
		if (node_pos[i].y<min.y) {min.y=node_pos[i].y;}	
	}
	count=0;
	targets=malloc(1);
	for (i=0;i<path_length(node_pos);i++) {
		if ((node_pos[i].x<min.x*3) || (node_pos[i].x>max.x-min.x*2)
		|| (node_pos[i].y<min.y*3) || (node_pos[i].y>max.y-min.y*2)) {
			targets=realloc(targets,sizeof(short)*(count+1));
			printf("- %d: %d at %dx%d\n",count,i,node_pos[i].x,node_pos[i].y);
			targets[count++]=i;
		}
	}
	targets=realloc(targets,sizeof(short)*(count+1));
	targets[count]=-1;
	return targets;
}

s_sim find_maxzoom(s_person *person,s_sim sim) {
	int i;
	s_path min,max,pos,newpos;
	float zoomx,zoomy,newzoom;
	min.x=SHRT_MAX;
	min.y=SHRT_MAX;
	max.x=0;
	max.y=0;
	for (i=0;i<sim.total;i++) {
		if (person[i].active==false) {continue;}
		if (person[i].wait_count!=0) {continue;}
		pos=person[i].pos;
		if (pos.x<min.x) {min.x=pos.x;}
		if (pos.x>max.x) {max.x=pos.x;}
		if (pos.y<min.y) {min.y=pos.y;}
		if (pos.y>max.y) {max.y=pos.y;}

	}
	zoomx=((float)max.x-(float)min.x)/OUTSIZE_X;
	zoomy=((float)max.y-(float)min.y)/OUTSIZE_Y;
	if (zoomx>zoomy) {newzoom=zoomx;}
	else {newzoom=zoomy;}
	if (newzoom<1.0) {newzoom=1.0;}
	if (newzoom>sim.zoomfakt) {sim.zoomfakt+=ZOOM_STEP;}
	sim.spritezoom=1+(sim.zoomfakt-1)/3; // was 2 in p4
	newpos.x=(max.x+min.x)/2;
	newpos.y=(max.y+min.y)/2;
	if (sim.view.x<newpos.x) {sim.view.x++;}
	if (sim.view.y<newpos.y) {sim.view.y++;}
	if (sim.view.x>newpos.x) {sim.view.x--;}
	if (sim.view.y>newpos.y) {sim.view.y--;}
	return sim;
}

char* target_str(int default_target) {
	if (default_target==-1) {return "random";}
	if (default_target==0) {return "left";}
	if (default_target==1) {return "up";}
	if (default_target==2) {return "right";}
	if (default_target==3) {return "down";}
	return NULL;
}
