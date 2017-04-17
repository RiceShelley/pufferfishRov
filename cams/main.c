#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <stdbool.h>
#include <string.h>
#include <SFML/Graphics.hpp>
#include <linux/videodev2.h>

/*
* <----Author---->
* name: Rice Shelley
* email: rootieDev@gmail.com
* note: I like when people 
* make sugestions about my code
* in email format please do so if
* you could 
*
* <----ABOUT---->
* web cam driver for puffer fish ROV
* grabs fram from USB cam in MJPEG format
* writes frame to file system as JPG
* reads frame back from file system and displays
* it in a SFML window
* 
* <---SPECS--->
* should work with any webcam that supports 
* MJPEG formats "should" - needs SFML library 
* to compile 
*
* what camera am I using -> my teacher bought it 
* it's a 13 dollor USB cam with an led for inspecting 
* drains and ITS WATER PROOF FOR 13$ what a world we live in :3
* 
* <---How I compile on lunix--->
* g++ main.c -lsfml-graphics -lsfml-window -lsfml-system
*/

// write frame to file system
void w_frame(void *bs, struct v4l2_buffer *bi) 
{
	int jf;
	if ((jf = open(".frame.jpg", O_WRONLY | O_CREAT, 0660)) < 0) {
		printf("could not write frame\n");	
		exit(-1);
	}
	write(jf, bs, bi->length);
	close(jf);
}

// update sfml window and display frames
void update_window(sf::RenderWindow *w) 
{	
	if (w->isOpen()) {
		sf::Event event;
		while (w->pollEvent(event)) {
			if (event.type == sf::Event::Closed)
				w->close();
		}	
		w->clear(sf::Color::Black);
		
		// render frame from webcam
		sf::Texture t;
		if (!t.loadFromFile(".frame.jpg")) {
			printf("Failed to load texture from file\n");
		}
		sf::Sprite s;
		s.setTexture(t);
		w->draw(s);

		// display renders
		w->display();
	} else {
		printf("Window closed\n");
		exit(-1);
	}
}

// Set up webcam for streaming and go into update loop
void webcam_stream(int *fd) 
{
	// query device for capabilities 
	struct v4l2_capability cap;
	if (ioctl(*fd, VIDIOC_QUERYCAP, &cap) < 0) {
		printf("error on query\n");
		exit(-1);
	}

	// set format type
	struct v4l2_format fmat;
	fmat.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmat.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
	fmat.fmt.pix.width = 640;
	fmat.fmt.pix.height = 480;

	if (ioctl(*fd, VIDIOC_S_FMT, &fmat) < 0) {
		printf("failed to establish format\n");
		exit(-1);
	}

	// inform device about buffers
	struct v4l2_requestbuffers br;
	br.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	br.memory = V4L2_MEMORY_MMAP;
	br.count = 1;	
	
	if (ioctl(*fd, VIDIOC_REQBUFS, &br) < 0) {
		printf("Error setting device buffs\n");
		exit(-1);
	}

	// allocate buffers
	struct v4l2_buffer bi;
	memset(&bi, 0, sizeof(bi));
	
	bi.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	bi.memory = V4L2_MEMORY_MMAP;
	bi.index = 0;
	
	if (ioctl(*fd, VIDIOC_QUERYBUF, &bi) < 0) {
		printf("Error allocating buffers\n");
		exit(-1);	
	}

	// map mem
	void *buff_start = mmap(
		NULL,
		bi.length,
		PROT_READ | PROT_WRITE,
		MAP_SHARED,
		*fd,
		bi.m.offset
	);

	if (buff_start == MAP_FAILED) {
		printf("mmmap failure\n");
		exit(-1);
	}
	
	memset(buff_start, 0, bi.length);

	// que buffer 
	memset(&bi, 0, sizeof(bi));

	bi.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	bi.memory = V4L2_MEMORY_MMAP;
	bi.index = 0;

	// Activate streaming
	int type = bi.type;
	if (ioctl(*fd, VIDIOC_STREAMON, &type) < 0) {
		printf("Error activating streaming\n");
		exit(-1);
	}

	// create sfml window for displaying frames
	sf::RenderWindow window(sf::VideoMode(640, 482), "cam");
	
	while (true) {
		// que buffer to be writting to 
		if (ioctl(*fd, VIDIOC_QBUF, &bi) < 0) {
			printf("Error putting buff in device que\n");
			exit(-1);
		}

		// wait for buff to be send back
		if (ioctl(*fd, VIDIOC_DQBUF, &bi) < 0) {
			printf("Error reciving buffer\n");
			exit(-1);
		}
		w_frame(buff_start, &bi);
		update_window(&window);
		// sleep cpu before next cycle
		usleep(1000);
	}

	// dectivate streaming
	if (ioctl(*fd, VIDIOC_STREAMOFF, &type) < 0) { 
		printf("Error turning stream off\n");	
		exit(-1);
	}
	
	printf("%s\n", cap.driver);	

}

int main(int argc, char **argv) 
{
	if (argc != 2)
		printf("Insufficient params\nex: ./a.out /dev/video1\n");
	int fd;
	if ((fd = open(argv[1], O_RDWR)) < 0) {
		printf("could not open device at %s\n", argv[1]);
		exit(-1);
	}
	webcam_stream(&fd);	
	close(fd);

}
