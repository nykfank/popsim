// Population Simulator, abstract version
// 2010-08-05, Nick Fankhauser

// todo:
// save the latest zoom image and use it if factor is unchanged
// after target is reached, go to new target or back to origin
// colors for moving in different directions? Color for density?

#include <stdlib.h>
#include <gd.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>

#define ZOOM_STEP 0.01	// increase or decrease per frame
#define FONT_SIZE 16.0 // font size for annotation
#define FONT_SIZE2 14.0 // font size for annotation
#define FONT_SPACE 20 // space in pixels between text lines
#define FONT_SPACE2 18 // space in pixels between text lines
#define OUTSIZE_X 720.0	 // output resolution X
#define OUTSIZE_Y 540.0 // output resolution Y
#define ACTIVATION_DISTANCE 100 // distance to next active object (was 50)
#define ACTIVATION_WAIT 25 // number of frames no one is activated (10 for lindenau)
#define WAIT_MIN 25*1 // min number of frames before start of activation
#define WAIT_MAX 25*40	// max number of frames before start of activation
#define REPATH_BORDER 600 // pathfinding border (10 was working fine, but used lots of memory)
#define COLLISION_INDEX_RED 255 // red color intensity in sprite used for collision detection
#define MAX_LINE_LEN 15  // maximum line length in path text file ("10184 10070")
#define MAX_NAME_LEN 200  // maximum line length in text file
#define STREET_SCALE 8 // by what factor was the street image scaled
#define JPEG_QUALITY 90 // jpeg quality percentage of output frames
#define SHOW_LAST_NUMBER 10
#define MAP_FILE "data/map_bern.jpg" // 19200x14400 city-map
#define PIN_FILE "data/pin10x10.png" // sprite image
#define PIN_FILE2 "data/pin15x15.png" // seleted sprite image
#define FONT_FILE "data/arial.ttf" // truetype font file
#define STREET_FILE "data/streetmap_bern2.png" //  4800x3600 street-map
#define PERSON_POS_FILE "data/person_pos.txt" // start position of persons
#define PERSON_NAME_FILE "data/person_names.txt" // names of persons
#define NODE_FILE "data/streetmap_bern2_nodesInt.txt" // positions of nodes
#define EDGE_FILE "data/streetmap_bern2_edgesInt.txt" // edges
#define SAVE_RAW 1 // save person positions at each frame

#include "popsim.h"
#include "popa_misc.h"

int main(int argc, char *argv[]) {
	int i,j;
	short *target_nodes,default_target,skip;
	unsigned char **usemap;
	bool is_activated,is_follower;
	s_person pi,*person;
	s_edge **edgemap;
	s_sim sim;
	gdImagePtr img,oimg,oimg2,pin,pin2,pini,simg,zpin,zpin2;
	s_path *node_pos,*edges;
	if (argc<4) {
		printf("Syntax: popsim <name/address> <-1=random/target_index> <skip>\n");
		exit(1);
	}
	sim.total=text_file_length(PERSON_NAME_FILE);
	srand(time(0));
	node_pos=load_path(NODE_FILE);
	printf("- Loaded %d nodes from %s\n",path_length(node_pos),NODE_FILE);
	edges=load_path(EDGE_FILE);
	printf("- Loaded %d edges from %s\n",path_length(edges),EDGE_FILE);
	node_pos=scale_nodes(node_pos);
	target_nodes=find_border_nodes(node_pos);
	printf("- %d targets found\n",nodepath_length(target_nodes));
	default_target=atoi(argv[2]);
	skip=atoi(argv[3]);
	if (target_str(default_target)==NULL) {printf("Invalid target number\n");exit(1);}
	printf("- Default target: %s\n",target_str(default_target));
	person=create_person_objects(sim,target_nodes,default_target);
	free(target_nodes);
	sim.follow=search_person(person,argv[1],sim.total);
	sim=parse_address(sim,person[sim.follow].name);
	printf("- Following: %s, %s (%i)\n",sim.start_name,sim.start_adr,sim.follow);
	printf("- Creating edgemap...\n");
	edgemap=create_edgemap(node_pos,edges);
	pin=load_image(PIN_FILE);
	pin2=load_image(PIN_FILE2);
	simg=load_image(STREET_FILE);
	img=load_image(MAP_FILE);
	usemap=allocate_usemap(img);
	sim.zoomfakt=1.0;
	sim.frame=0;
	sim.view=person[sim.follow].pos;
	sim.activated=malloc(sizeof(int)*sim.total);
	sim.num_activated=0;
	sim.current_adr=malloc(1);
	while (true) {
		sim=find_maxzoom(person,sim);
		sim.moving=0;sim.inactive=0;sim.blocked=0;sim.waiting=0;sim.offpath=0;sim.finished=0;
		zpin=gdImageCreateTrueColor(pin->sx*sim.spritezoom,pin->sy*sim.spritezoom);
		zpin2=gdImageCreateTrueColor(pin2->sx*sim.spritezoom,pin2->sy*sim.spritezoom);
		gdImageCopyResampled(zpin,pin,0,0,0,0,zpin->sx,zpin->sy,pin->sx,pin->sy);
		gdImageCopyResampled(zpin2,pin2,0,0,0,0,zpin2->sx,zpin2->sy,pin2->sx,pin2->sy);
		for (i=0;i<sim.total;i++) {
			pi=person[i];
			if (pi.finished) {sim.finished++;continue;}
			is_follower=(i==sim.follow);
			if ((sim.frame<ACTIVATION_WAIT) && (!is_follower)) {sim.inactive++;continue;}
			if (pi.on_path) {pini=zpin2;} else {pini=zpin;}
			if (!pi.active) { // inactive person
				is_activated=scan_region(pi.startpos,usemap,img);
				if ((is_activated) || (is_follower)) {
					pi.active=true;
					pi.node1=-1;
					pi.node2=nearest_node(node_pos,pi.startpos);
					pi.nodepos=node_pos[pi.node2];
					if (is_follower) {
						pi.wait_count=0;
						sim.activated[sim.num_activated++]=i;
					}
				} else {sim.inactive++;continue;}
			}
			if (pi.wait_count>0) {
				pi.wait_count--;
				sim.waiting++;
				person[i]=pi;
				if (pi.wait_count==0) {sim.activated[sim.num_activated++]=i;}
				continue;
			}
			unmark_map(pi.pos,pini,usemap,img);
			if (!pi.on_path) { // off-path
				sim.offpath++;
				if ((pi.pos.x==pi.nodepos.x) && (pi.pos.y==pi.nodepos.y)) {
					pi.on_path=true;
				} else {pi.pos=directional_move(pi);}
			} else { // on-path
				if (next_pos(edgemap,pi).x==-1) { // end of path
					if ((pi.node1>-1) && (pi.node2>-1)) { // free last position
						edgemap[pi.node1][pi.node2].used[pi.path_pos]=false;
						edgemap[pi.node1][pi.node2].use_count--;
					}
					if (pi.node2==pi.target_node) { // target node reached
						pi.finished=true;
						person[i]=pi;
						continue;
					}
					pi=next_path(node_pos,edgemap,pi);
					edgemap=check_path(edgemap,simg,node_pos,pi);
					if (does_path_exists(edgemap,pi)) {
						edgemap[pi.node1][pi.node2].use_count++;
					} else {pi=previous_path(node_pos,pi);}
				}
				if (!is_path_used(edgemap,pi)) { // next path position free
					pi.pos=next_pos(edgemap, pi);
					if (pi.path_pos>-1) {
						edgemap[pi.node1][pi.node2].used[pi.path_pos]=false;
					}
					edgemap[pi.node1][pi.node2].used[pi.path_pos+1]=true;
					pi.path_pos++;
					sim.moving++;
				} else {sim.blocked++;}
			}
			mark_map(pi.pos,pini,usemap,img);
			person[i]=pi;
		}
		gdImageDestroy(zpin);
		gdImageDestroy(zpin2);
		sim.frame++;
		logg(sim);
		if ((sim.frame-1) % skip!=0) {continue;}
		free(sim.current_adr);
		sim.current_adr=nearest_address(person,sim);
		oimg=create_frame(img,usemap,sim);
		oimg2=gdImageCreateTrueColor(OUTSIZE_X,OUTSIZE_Y);
		gdImageCopyResampled(oimg2,oimg,0,0,0,0,oimg2->sx,oimg2->sy,oimg->sx,oimg->sy);
		oimg2=annotate(oimg2,sim,person);
		save_frame(oimg2,sim);
		if (SAVE_RAW==1) {
			save_positions(person,sim);
			save_identifiers(person,sim);
		}
		// loop clean up
		gdImageDestroy(oimg);
		gdImageDestroy(oimg2);
		if (sim.offpath+sim.moving==0) {break;}
	}
	// final clean up
	for (i=0;i<img->sx;i++) {free(usemap[i]);}
	free(usemap);
	for (i=0;i<sim.total;i++) {free(person[i].name);}
	free(person);
	for (i=0;i<path_length(node_pos);i++) {
		for (j=0;j<path_length(node_pos);j++) {
			if (edgemap[i][j].alloc) {
				free(edgemap[i][j].path);
				free(edgemap[i][j].used);
			}
		}
		free(edgemap[i]);
	}
	free(edgemap);
	free(sim.activated);
	free(sim.current_adr);
	free(sim.start_name);
	free(sim.start_adr);
	free(node_pos);
	free(edges);
	gdImageDestroy(pin);
	gdImageDestroy(pin2);
	gdImageDestroy(img);
	gdImageDestroy(simg);
}

