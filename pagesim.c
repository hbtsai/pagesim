/*
   Page Replacement Algorithms
   Author: Selby Kendrick
Contributor: Hong-Bin Tsai
   Description: Simulation of common page replacement algorithms used by
   operating systems to manage memory usage
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <getopt.h>
#include <sys/queue.h>
#include "pagesim.h"



/*
 * TODO List:
 *
 * 3. add LRU-K
 * 4. add Adaptive Replacement Cache
 * 5. add R/W ratio & dirty bit.
 *
 *
 * Wish List:
 * 1. add hotness analysis from real trace.
 * 2. add distribution of hot pages.
 *
 *
 */


// in mu-seconds
const double HDD_READ_LATENCY = 5000; // 5 ms, 5000 us, 5000000 ns,
const double HDD_WRITE_LATENCY = 6000; // 6 ms
const double SSD_READ_LATENCY = 25; 
const double SSD_WRITE_LATENCY = 350;
const double PCM_READ_LATENCY = 0.065; // 65ns
const double PCM_WRITE_LATENCY = 0.5; // 500ns
const double STT_RAM_READ_LATENCY = 0.006; // 6ns
const double STT_RAM_WRITE_LATENCY = 0.013; // 13ns


/**
 * Configuration variables
 */
int num_frames = 10; // Number of avaliable pages in page tables
int page_ref_upper_bound = -1; //2*num_frames Largest page reference
int max_page_calls = -1;//1000*num_frames; // Max number of page refs to test
int swap_mode=0; // enable swapping (between frame list and victim list)
int debug_flag = 0; // Debug bool, 1 shows verbose output
int printrefs = 0; // Print refs bool, 1 shows output after each page ref
int _print_page_ref_stat =0;
int _num_x=10;
time_t _start_time;
int _num_of_hotpages=-1;
int _window_size=-1;
FILE *_fp = NULL; // export page number referenced
char EXPORT_FILE[]="page_reference_list.csv";

int _head_hot=0;
int _tail_hot=0;
int _mid_hot=0;
int _dual_head_hot=0;

/**
 * Array of algorithm functions that can be enabled
 */
Algorithm algos[11] = { {"OPTIMAL", &OPTIMAL, 0, NULL},
                       {"RANDOM", &RANDOM, 0, NULL},
                       {"FIFO", &FIFO, 0, NULL},
                       {"LRU", &LRU, 0, NULL},
                       {"CLOCK", &CLOCK, 0, NULL},
                       {"NFU", &NFU, 0, NULL},
                       {"AGING", &AGING, 0, NULL},
                       {"LOG", &LOG, 0, NULL},
                       {"LOG_NOWIN", &LOG_NOWIN, 0, NULL},
                       {"LRU2", &LRU2, 0, NULL},
                       {"LRU3", &LRU3, 0, NULL}
};

/**
 * Runtime variables, don't touch
 */
int counter = 0; // "Time" as number of loops calling page_refs 0..._num_refs (used as i in for loop)
int last_page_ref = -1; // Last ref
size_t num_algos = 0; // Number of algorithms in algos, calculated in init()
int *optimum_find_test;
int _num_refs = 0; // Number of page refs in page_refs list
char _trace_file[256]={};

static struct option long_options[] = {
	{"algo", required_argument, 0, 'a'},
	{"multi", required_argument, 0, 'x'},
	{"frames", required_argument, 0, 'f'},
	{"dist_page", required_argument, 0, 'p'},
	{"hotness", required_argument, 0, 'h'},
	{"ref_stat", no_argument, &_print_page_ref_stat, 1},
	{"window", required_argument, 0, 'w'},
	{"verbose", no_argument, &printrefs, 1},
	{"debug", no_argument, &debug_flag, 1},
	{0, 0, 0, 0}
};

/**
 * int main(int argc, char *argv[])
 *
 * @param argc {int} number of commandline terms eg {'./pagesim' => argc=1}
 * @param argv {char **} arguments passed in
 *
 * Run algo if given correct arguments, else terminate with error
 */
int main ( int argc, char *argv[] )
{
        if ( argc < 3)
                print_help(argv[0]);

		int long_index = 0;
		int opt;
		int i=0;
		char* token;
		char algo_str[128]={0};
		char delim[2]={0};
		delim[0]=' ';
		delim[1]=',';


        while((opt = getopt_long(argc, argv, "a:f:w:vdsrx:p:h:t:HTMD", long_options, &long_index)) != -1)
        {

			switch(opt)
			{
				case 'a':
					strcpy(algo_str, optarg);
					token = strtok(algo_str, delim);
					while(token)
					{
						if(strcmp(token, "LRU") == 0)
							algos[3].selected = 1;
						else if(strcmp(token, "LOG") == 0)
							algos[7].selected = 1;
						else if(strcmp(token, "CLOCK") == 0)
							algos[4].selected = 1;
						else if(strcmp(token, "NFU") == 0)
							algos[5].selected = 1;
						else if(strcmp(token, "AGING") == 0)
							algos[6].selected = 1;
						else if(strcmp(token, "RANDOM") == 0)
							algos[1].selected = 1;
						else if(strcmp(token, "FIFO") ==0)
							algos[2].selected = 1;
						else if(strcmp(token, "OPT") ==0)
							algos[0].selected = 1;
						else if(strcmp(token, "LRU2") ==0)
							algos[9].selected = 1;
						else if(strcmp(token, "LRU3") ==0)
							algos[10].selected = 1;
						else
							fprintf(stderr, "unrecognized or unsupported algorithm: %s\n", token);
						token = strtok(0, delim);
					}

					break;
				case 'f':
					num_frames = atoi(optarg);
					if ( num_frames < 1 )
               		{
               		        num_frames = 1;
               		        printf( "Number of page frames must be at least 1, setting to 1\n");
               		}
					break;
				case 'p':
					page_ref_upper_bound = atoi(optarg);
					if(page_ref_upper_bound<num_frames)
					{
						page_ref_upper_bound=num_frames<<1;
						fprintf(stderr, "number of distinct pages should be larger than number of frames, set to %d\n", page_ref_upper_bound);
					}
					break;
				case 'w':
					_window_size=atoi(optarg);
					break;
				case 'r':
					_print_page_ref_stat = 1;
					break;
				case 'v':
					printrefs = 1;
					break;
				case 'd':
					debug_flag = 1;
					break;
				case 's':
					swap_mode = 1;
					break;
				case 'x':
					_num_x=atoi(optarg);
					break;
				case 'h':
					/*
					 * 10 = 10% data takes 90% refs
					 * 20 = 20% data takes 80% refs
					 * 30 = 30% data takes 70% refs
					 * 40 = 40% data takes 60% refs
					 */
					_num_of_hotpages = atoi(optarg);
					break;
				case 't':
					strcpy(_trace_file, optarg);
					if(access(_trace_file, R_OK)!=0)
					{
						perror("access()");
						exit(-1);
					}
					break;
				case 'H':
					_head_hot=1;
					break;
				case 'T':
					_tail_hot=1;
					break;
				case 'M':
					_mid_hot=1;
					break;
				case 'D':
					_dual_head_hot=1;
					break;
				default:
					print_help(argv[0]);
					break;
			}
		}

		if(_head_hot + _tail_hot + _mid_hot + _dual_head_hot > 1)
		{
			fprintf(stderr, "[ERR] one distribution a time please!! \n");
			exit(-1);
		}
		
		if(strlen(algo_str)==0)
			for(i=0; i< sizeof(algos)/sizeof(Algorithm); i++)
				algos[i].selected = 1;

        init();
		event_loop();
        cleanup();
        return 0;
}

void print_page_ref_stat()
{
	Page_Ref* p=NULL;
	int *page_ref_num=NULL;
	int refs=0;
	page_ref_num = malloc(sizeof(int) * page_ref_upper_bound);
	LIST_FOREACH(p, &page_refs, pages )
	{
		page_ref_num[p->page_num]++;
		refs++;
		if(refs==max_page_calls)
			break;
	}

	int i=0;
	for(i=0; i<page_ref_upper_bound; i++)
		printf("page[%02d] refs: %6d, percentage:%f\n", i, page_ref_num[i], (double)page_ref_num[i]/(double)refs);

	printf("total number of references: %d\n", refs);
	free(page_ref_num);
}

int read_page_refs()
{

	FILE *fp = NULL;
	char strPage[8]={};
	int refs=1;
	if( (fp = fopen(_trace_file, "r") ) == NULL)
	{
		perror("fopen()");
		exit(-1);
	}
	
	LIST_INIT(&page_refs);
	Page_Ref *page = NULL;
	Page_Ref *headp = NULL;
	headp = malloc(sizeof(Page_Ref));
	if(fgets(strPage, 1024, fp)!=NULL)
	{
		headp->page_num = (int) atoi(strPage);
		LIST_INSERT_HEAD(&page_refs, headp, pages);
		while(fgets(strPage, 1024, fp)!=NULL)
		{
			if(strlen(strPage)>0)
			{
				page = malloc(sizeof(Page_Ref));
				page->page_num = (int) atoi(strPage);
	    	    LIST_INSERT_AFTER(headp, page,  pages);
				headp = headp->pages.le_next;
				refs++;
			}
		}
	}


	
	max_page_calls = refs;

	/* init optimum_find_test for optimal algo */
    optimum_find_test = (int*)malloc(page_ref_upper_bound*sizeof(int));
        for(int i = 0; i < page_ref_upper_bound; ++i)
                optimum_find_test[i] = -1;

		int all_found=0;
        while(all_found == 0)
        { // generate new refs until one of each have been added to list
                LIST_INSERT_AFTER(page, gen_ref(NULL, 0), pages);
                page = page->pages.le_next;
                optimum_find_test[page->page_num] = 1;
                all_found = 1;
                for(int i = 0; i < page_ref_upper_bound; ++i)
                { // see if we've got them all yet
                        if(optimum_find_test[i] == -1)
                        {
                                all_found = 0;
                                break;
                        }
                }
        }

	return 0;
}

/**
 * int init()
 *
 * Initialize lists and variables
 *
 * @return 0
 */
int init()
{

	time(&_start_time);
	max_page_calls = num_frames * pow((double)2, (double)_num_x);
	//page_ref_upper_bound = num_frames<<1;
	if(page_ref_upper_bound < 0)
	{
		fprintf(stderr, ">>> # of distinct pages not set, set to %d\n", num_frames<<1);
		page_ref_upper_bound=num_frames<<1;
	}

	if(_num_of_hotpages < 0)
	{
		fprintf(stderr, ">>> # of hot pages not set, set to 0 \n");
		_num_of_hotpages = 0;
	}
	else
		_num_of_hotpages = ((double)_num_of_hotpages/100.0)*(page_ref_upper_bound);

	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	srand((int)ts.tv_nsec);


	if(strlen(_trace_file)>0)
		read_page_refs();
	else
		gen_page_refs();


	if(_print_page_ref_stat)
		print_page_ref_stat();
    // Calculate number of algos
    num_algos = sizeof(algos)/sizeof(Algorithm);
    size_t i = 0;
    for (i = 0; i < num_algos; ++i)
            algos[i].data = create_algo_data_store();
    return 0;
}

/*
 * return 0 if time1 = time2
 * return -1 if time1 < time2
 * return 1 if time1 > time2
 */
int compare_time(struct timespec time1, struct timespec time2)
{
	int ret=0;
	if(time1.tv_sec > time2.tv_sec)
		ret = 1;
	else if(time1.tv_sec < time2.tv_sec)
		ret = -1;
	else if(time1.tv_sec == time2.tv_sec)
	{
		if(time1.tv_nsec > time2.tv_nsec)
			ret = 1;
		else if(time1.tv_nsec < time2.tv_nsec)
			ret = -1;
		else 
			ret = 0;
	}
	return ret;
}

/**
 * void gen_page_refs()
 *
 * Generate all page refs to use in tests
 *
 * @return 0
 */
void gen_page_refs()
{
	int hotpages[_num_of_hotpages];
	int i=0, j=0;
	int dupe=0;


	/* select non-duplicated  hot pages */
	fprintf(stderr, "hot pages: ");
	for(i=0; i<_num_of_hotpages; i++)
	{
		while(1)
		{
			hotpages[i]=rand() % page_ref_upper_bound;
			dupe=0;

			for(j=0; j<i; j++)
			{
				if(hotpages[j]==hotpages[i])
				{
					dupe=1;
					break;
				}
			}
			if(dupe)
				continue;
			else
				break;
		}

		fprintf(stderr, "%d ", hotpages[i]);
	}
	fprintf(stderr, "\n");

        _num_refs = 0;
        LIST_INIT(&page_refs);
        Page_Ref *page = gen_ref(hotpages, _num_of_hotpages);
        LIST_INSERT_HEAD(&page_refs, page, pages);
        while(_num_refs < max_page_calls)
        { // generate a page ref up to max_page_calls and add to list


			if(_head_hot && _num_refs > max_page_calls/2)
			{
                LIST_INSERT_AFTER(page, gen_ref(NULL, 0), pages);
			}
			else if(_tail_hot && _num_refs < max_page_calls / 2 )
			{
                LIST_INSERT_AFTER(page, gen_ref(NULL, 0), pages);
			}
			else if(_mid_hot && (_num_refs < max_page_calls/4 || _num_refs > max_page_calls*3/4))
			{
                LIST_INSERT_AFTER(page, gen_ref(NULL, 0), pages);
			}
			else if(_dual_head_hot &&  _num_refs > max_page_calls/4 && _num_refs < max_page_calls*3/4)
			{
                LIST_INSERT_AFTER(page, gen_ref(NULL, 0), pages);
			}
			else
			{
                LIST_INSERT_AFTER(page, gen_ref(hotpages, _num_of_hotpages), pages);
			}

                page = page->pages.le_next;
                _num_refs++;
        }

        // we need look-ahead for Optimal algorithm
        int all_found = 0;
        optimum_find_test = (int*)malloc(page_ref_upper_bound*sizeof(int));

        for(i = 0; i < page_ref_upper_bound; ++i)
                optimum_find_test[i] = -1;

        while(all_found == 0)
        { // generate new refs until one of each have been added to list
                LIST_INSERT_AFTER(page, gen_ref(hotpages, _num_of_hotpages), pages);
                page = page->pages.le_next;
                optimum_find_test[page->page_num] = 1;
                all_found = 1;
                for(i = 0; i < page_ref_upper_bound; ++i)
                { // see if we've got them all yet
                        if(optimum_find_test[i] == -1)
                        {
                                all_found = 0;
                                break;
                        }
                }
                _num_refs++;
        }
        return;
}

/**
 * Page_Ref* gen_ref()
 *
 * generate a random page ref within bounds
 *
 * @return {Page_Ref*}
 */
Page_Ref* gen_ref(int* hotpages, int nHotpages)
{
	int page_num = -1;
	int i=0;
//	int found=0;
//	page_num = rand() % page_ref_upper_bound;

	if(nHotpages>0 && 
			((double)rand()/(double)(RAND_MAX)) < (1- (double)((double)(nHotpages)/(double)(page_ref_upper_bound))))
	{
			page_num = hotpages[rand() % nHotpages];
	}
	else
	{
		while(1)
		{
			page_num = rand() % page_ref_upper_bound;
			for(i=0; i<nHotpages; i++)
			{
				if(page_num == hotpages[i])
					break;
			}
			if(i==nHotpages)
				break;
		}
	}

//	fprintf(stderr, "dice = %f\n", (double)rand()/(double)RAND_MAX);

    Page_Ref *page = malloc(sizeof(Page_Ref));
    page->page_num =  page_num;
	//fprintf(stderr, "%s:%d page_num==%d\n", __FILE__, __LINE__, page->page_num);
    return page;
}

/**
 * Algorithm_Data* create_algo_data_store(int num_frames)
 *
 * Creates an empty Algorithm_Data to init an Algorithm
 *
 * @return {Algorithm_Data*} empty Algorithm_Data struct for an Algorithm
 */
Algorithm_Data *create_algo_data_store()
{
        Algorithm_Data *data = malloc(sizeof(Algorithm_Data));
        data->hits = 0;
        data->misses = 0;
		data->swap_in = 0;
		data->swap_out = 0;
		data->total_ref_count = 0;
        data->last_victim = NULL;
        /* Initialize Lists */
        TAILQ_INIT(&(data->page_ref_log));
        TAILQ_INIT(&(data->page_window_log));
        LIST_INIT(&(data->page_table));
        LIST_INIT(&(data->victim_list));
        LIST_INIT(&(data->swap_list));
        /* Insert at the page_table. */
        Frame *framep = create_empty_frame(0);
        LIST_INSERT_HEAD(&(data->page_table), framep, frames);
        /* Build the rest of the list. */
        size_t i = 0;
        for (i = 1; i < num_frames; ++i)
        {
                LIST_INSERT_AFTER(framep, create_empty_frame(i), frames);
                framep = framep->frames.le_next;
        }
        return data;
}

/**
 * Frame* create_empty_frame(int num_frames)
 *
 * Creates an empty Frame for page table list
 *
 * @return {Frame*} empty Frame entry for page table list
 */
Frame* create_empty_frame(int index)
{
        Frame *framep = malloc(sizeof(Frame));
        framep->index = index;
        framep->page = -1;
        clock_gettime(CLOCK_REALTIME, &framep->time);
        framep->extra = 0;
        return framep;
}

/**
 * int event_loop()
 *
 * page all selected algorithms with input ref
 *
 * @param page_ref {int} page to ref
 *
 * @return 0
 */
int event_loop()
{
		int page_num = 0;
        counter = 0;
        while(counter < max_page_calls)
        {
				page_num =  get_ref();
                page(page_num);
                ++counter;
				export(counter, page_num);
        }
        size_t i = 0;
        for (i = 0; i < num_algos; i++)
        {
                if(algos[i].selected==1) {
                        print_summary(algos[i]);
                }
        }
        return 0;
}

/**
 * int get_ref()
 *
 * get a random ref
 *
 * @return {int}
 */
int get_ref()
{
        if (page_refs.lh_first != NULL)
        { // pop Page_Ref off page_refs
                int page_num = page_refs.lh_first->page_num;
                LIST_REMOVE(page_refs.lh_first, pages);
                return page_num;
        }
        else
        { // just in case
                return rand() % page_ref_upper_bound;
        }
}

/**
 * int page()
 *
 * page all selected algorithms with input ref
 *
 * @param page_ref {int} referenced page number
 *
 * @return 0
 */
int page(int page_ref)
{
        last_page_ref = page_ref;
        size_t i = 0;
        for (i = 0; i < num_algos; i++)
        {
                if(algos[i].selected==1) {
                        algos[i].algo(algos[i].data);
                        if(printrefs == 1)
                                print_stats(algos[i]);
                }
        }

        return 0;
}

int export(int counter, int page_num)
{
	if(_fp==NULL)
		_fp = fopen(EXPORT_FILE, "w+");


	fprintf(_fp, "%d,%d\n", counter, last_page_ref);


	return 0;
}

int swap_in(struct Frame_List *swap_list)
{
	int hit = 0;
	struct Frame *fr=NULL;
	LIST_FOREACH(fr, swap_list, frames)
	{
		if(fr->page == last_page_ref)
		{
			hit = 1;
			clock_gettime(CLOCK_REALTIME, &fr->time);
			break;
		}
	}

	return hit;
}

int swap_out(struct Frame_List *swap_list, struct Frame *frame)
{
        if(debug_flag)
                printf("swap out index: %d, Page: %d\n", frame->index, frame->page);
        struct Frame *swap = malloc(sizeof(Frame));
        *swap = *frame;
        swap->index = -1;
        LIST_INSERT_HEAD(swap_list, swap, frames);
        return 0;
}

/**
 * int add_victim(struct Frame_List *victim_list, struct Frame *frame)
 *
 * Add victim frame evicted from page table to list of victims
 *
 * @param index {Frame_List} list of victims
 * @param page {Frame} page frame evicted
 *
 * @retun 0
 */
int add_victim(struct Frame_List *victim_list, struct Frame *frame)
{
        if(debug_flag)
                printf("Victim index: %d, Page: %d\n", frame->index, frame->page);
        struct Frame *victim = malloc(sizeof(Frame));
        *victim = *frame;
        victim->index = -1;
        LIST_INSERT_HEAD(victim_list, victim, frames);
        return 0;
}


/**
 * int OPTIMAL(Algorithm_Data *data)
 *
 * OPTIMAL Page Replacement Algorithm
 *
 * @param *data {Algorithm_Data} struct holding algorithm data
 *
 * return {int} did page fault, 0 or 1
 */
int OPTIMAL(Algorithm_Data *data)
{
        Frame *framep = data->page_table.lh_first,
              *victim = NULL;
        int fault = 0;
data->total_ref_count++;
        /* Find target (hit), empty page index (miss), or victim to evict (miss) */
        while (framep != NULL && framep->page > -1 && framep->page != last_page_ref) {
                framep = framep->frames.le_next;
        }
        if(framep == NULL)
        { // It's a miss, find our victim
                size_t i,j;
                for(i = 0; i < page_ref_upper_bound; ++i)
                        optimum_find_test[i] = -1;
                Page_Ref *page = page_refs.lh_first;
                int all_found = 0;
                j = 0;
                //optimum_find_test = malloc(sizeof(int)*page_ref_upper_bound);
                while(all_found == 0)
                {
                        if(optimum_find_test[page->page_num] == -1)
						{
//							fprintf(stderr, "optimum %d set to %zu\n", page->page_num, j);
                                optimum_find_test[page->page_num] = j++;
						}
                        all_found = 1;
//						fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
                        for(i = 0; i < page_ref_upper_bound; ++i)
						{
                                if(optimum_find_test[i] == -1)
                                {
//									fprintf(stderr, "optimum %zu = -1\n", i);
                                        all_found = 0;
                                        break;
                                }
						}
//						fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
						page = page->pages.le_next;
//						fprintf(stderr, "%s:%d page =%p\n", __FILE__, __LINE__, page);
							
                }
//				fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
                framep = data->page_table.lh_first;
                while (framep != NULL) {
                        if(victim == NULL || optimum_find_test[framep->page] > optimum_find_test[victim->page])
                        { // No victim yet or page used further in future than victim
                                victim = framep;
                        }
                        framep = framep->frames.le_next;
                }
                if(debug_flag) printf("Victim selected: %d, Page: %d\n", victim->index, victim->page);
                add_victim(&data->victim_list, victim);
                victim->page = last_page_ref;
				clock_gettime(CLOCK_REALTIME,&victim->time );
                victim->extra = counter;
                fault = 1;
        }
        else if(framep->page == -1)
        { // Use free page table index
                framep->page = last_page_ref;
				clock_gettime(CLOCK_REALTIME,&framep->time );
                framep->extra = counter;
                fault = 1;
        }
        else if(framep->page == last_page_ref)
        { // The page was found! Hit!
				clock_gettime(CLOCK_REALTIME,&framep->time );
                framep->extra = counter;
        }
        if(debug_flag)
        {
                printf("Page Ref: %d\n", last_page_ref);
                for (framep = data->page_table.lh_first; framep != NULL; framep = framep->frames.le_next)
                        printf("Slot: %d, Page: %d, Time used: %d\n", framep->index, framep->page, framep->extra);
        }

		if(_window_size > 0)
		{
			if(data->total_ref_count >= _window_size && 
					(data->total_ref_count + _window_size) < max_page_calls )
			{
				if(fault == 1) data->misses++; else data->hits++;
			}
		}
		else
				if(fault == 1) data->misses++; else data->hits++;
        return fault;
}

/**
 * int RANDOM(Algorithm_Data *data)
 *
 * RANDOM Page Replacement Algorithm
 *
 * @param *data {Algorithm_Data} struct holding algorithm data
 *
 * return {int} did page fault, 0 or 1
 */
int RANDOM(Algorithm_Data *data)
{
        struct Frame *framep = data->page_table.lh_first,
                     *victim = NULL;
        int rand_victim = rand() % num_frames;
        int fault = 0;
		data->total_ref_count++;
        /* Find target (hit), empty page index (miss), or victim to evict (miss) */
        while (framep != NULL && framep->page > -1 && framep->page != last_page_ref) {
                if(framep->index == rand_victim) // rand
                        victim = framep;
                framep = framep->frames.le_next;
        }
        if(framep == NULL)
        { // It's a miss, kill our victim
                if(debug_flag) printf("Victim selected: %d, Page: %d\n", victim->index, victim->page);
                add_victim(&data->victim_list, victim);
                victim->page = last_page_ref;
				clock_gettime(CLOCK_REALTIME,&victim->time );
                victim->extra = counter;
                fault = 1;
        }
        else if(framep->page == -1)
        { // Use free page table index
                framep->page = last_page_ref;
				clock_gettime(CLOCK_REALTIME,&framep->time );
                framep->extra = counter;
                fault = 1;
        }
        else if(framep->page == last_page_ref)
        { // The page was found! Hit!
				clock_gettime(CLOCK_REALTIME,&framep->time );
                framep->extra = counter;
        }
        if(debug_flag)
        {
                printf("Page Ref: %d\n", last_page_ref);
                for (framep = data->page_table.lh_first; framep != NULL; framep = framep->frames.le_next)
                        printf("Slot: %d, Page: %d, Time used: %d\n", framep->index, framep->page, framep->extra);
        }

		if(_window_size > 0)
		{
			if(data->total_ref_count >= _window_size && 
					(data->total_ref_count + _window_size) < max_page_calls )
			{
    	    if(fault == 1) data->misses++; else data->hits++;
			}
		}
		else
    	    if(fault == 1) data->misses++; else data->hits++;

        return fault;
}

/**
 * int FIFO(Algorithm_Data *data)
 *
 * FIFO Page Replacement Algorithm
 *
 * @param *data {Algorithm_Data} struct holding algorithm data
 *
 * return {int} did page fault, 0 or 1
 */
int FIFO(Algorithm_Data *data)
{
        struct Frame *framep = data->page_table.lh_first,
                     *victim = NULL;
        int fault = 0;
		data->total_ref_count++;
        /* Find target (hit), empty page index (miss), or victim to evict (miss) */
        while (framep != NULL && 
				framep->page > -1 && 
				framep->page != last_page_ref) 
		{
                if(victim == NULL || 
					compare_time(framep->time, victim->time)==1)
                { // No victim yet or frame older than victim
                        victim = framep;
                }
                framep = framep->frames.le_next;
        }
        /* Make a decision */
        if(framep == NULL)
        { // It's a miss, kill our victim
                if(debug_flag) printf("Victim selected: %d, Page: %d\n", victim->index, victim->page);
                add_victim(&data->victim_list, victim);
                victim->page = last_page_ref;
				clock_gettime(CLOCK_REALTIME,&victim->time );
                victim->extra = counter;
                fault = 1;
        }
        else if(framep->page == -1)
        { // Can use free page table index
                framep->page = last_page_ref;
				clock_gettime(CLOCK_REALTIME,&framep->time );
                framep->extra = counter;
                fault = 1;
        }
        else if(framep->page == last_page_ref)
        { // The page was found! Hit!
				//clock_gettime(CLOCK_REALTIME,&framep->time );
                framep->extra = counter;
        }

		if(_window_size > 0)
		{
			if(data->total_ref_count >= _window_size && 
					(data->total_ref_count + _window_size) < max_page_calls )
			{
			    if(fault == 1) data->misses++; else data->hits++;
			}
		}
		else
			if(fault == 1) data->misses++; else data->hits++;
        return fault;
}


int LOG_NOWIN(Algorithm_Data *data)
{
        struct Frame *framep = data->page_table.lh_first, // lh_first = first element of queue 
                     *victim = NULL;

		struct Page_Log *page = NULL;
        int fault = 0;
		data->total_ref_count++;


		/* increase page reference count by 1 */
		int counted=0; 
		struct Page_Log *pg = NULL;
		TAILQ_FOREACH(pg, &data->page_ref_log, pages)
		{
			if(pg->page_num == last_page_ref)
			{
				pg->ref_count++;
				counted = 1;
				break;
			}
		}

		if(counted ==0) // if this page is never referenced before, add to log
		{
			page = malloc(sizeof(struct Page_Log));
			page->page_num = last_page_ref;
			page->ref_count = 1;
			TAILQ_INSERT_TAIL(&data->page_ref_log, page, pages);
			//data->page_ref_log_size++;
		}
/*
		page = malloc(sizeof(struct Page_Log));
		page->page_num = last_page_ref;
		// insert to window log here
		TAILQ_INSERT_TAIL(&data->page_window_log, page, pages);
		data->page_ref_log_size++;
		if(_window_size > 0 && data->page_ref_log_size > _window_size)
		{
			page = data->page_window_log.tqh_first;
			TAILQ_FOREACH(pg, &data->page_ref_log, pages)
			{
				if(pg->page_num == page->page_num)
				{
					pg->ref_count--;
					break;
				}
			}
			if(page != NULL) // just in case 
			{
				TAILQ_REMOVE(&data->page_window_log, data->page_window_log.tqh_first, pages);
				data->page_ref_log_size--;
			}
		}
*/

		/*
		 * search log list for ref_count of page num contained by frame list.
		 * pick the one with lowest hotness to evict. 
		 */
		double hotness = 0.0;
		double min_hotness=1.0;
		while(framep != NULL &&
				framep->page > -1 &&
				framep->page != last_page_ref)
		{

			/*
			if( _window_size > 0 && data->page_ref_log_size < _window_size)
			{
				if(victim==NULL || compare_time(framep->time, victim->time)==-1)
					victim = framep;
			}
			else
			{
			*/
				TAILQ_FOREACH(pg, &data->page_ref_log, pages)
				{
					if(framep->page == pg->page_num)
					{
						//fprintf(stderr, "%s:%d page %d hotness = %f\n", __FILE__, __LINE__, pg->page_num, (double)pg->ref_count/(double)data->total_ref_count);
						hotness = (double)pg->ref_count/(double)data->total_ref_count;
						if(hotness < min_hotness)
						{
							min_hotness = hotness;
							victim = framep;
						}
					}
				}
			//}

            framep = framep->frames.le_next;
		}

        /* Make a decision */
        if(framep == NULL) 
        { // It's a miss, kill our victim, need to swap


			/*
			 * search swap list, if hit in swap, swap-in, else swap-out
			 * in this implementation, DAX is not supported. 
			 * victim is pointing to memory page to be evicted. 
			 * 1. add victim to victim list
			 * 1.2. check swap_list and swap-in if found in swap_list
			 * 1.3. if hit in swap, renew the reference time in swap.
			 * 1.5. if miss in swap, swap-out victim page to swap_list
			 * 2. change victim page's value to referenced page
			 */


			if(debug_flag) printf("Victim selected: %d, Page: %d\n", victim->index, victim->page);
			add_victim(&data->victim_list, victim);


			/*
			if(swap_in(&data->swap_list)==1) // last_page_ref is found in swap_list
				data->swap_in++;
			else
			{
				swap_out(&data->swap_list, victim);
				data->swap_out++;
			}
			*/

			victim->page = last_page_ref;
			clock_gettime(CLOCK_REALTIME,&victim->time );
			victim->extra = counter;
			fault = 1;


        }
        else if(framep->page == -1)
        { // Can use free page table index
                framep->page = last_page_ref;
				clock_gettime(CLOCK_REALTIME,&framep->time );
                framep->extra = counter;
                fault = 1;
        }
        else if(framep->page == last_page_ref)
        { // The page was found! Hit!
				clock_gettime(CLOCK_REALTIME,&framep->time );
                framep->extra = counter;
        }

		/*
		if(_window_size > 0)
		{
			if(data->total_ref_count >= _window_size && 
					(data->total_ref_count + _window_size) < max_page_calls )
			{
				if(fault == 1) data->misses++; else data->hits++;
			}
		}
		else
		*/
				if(fault == 1) data->misses++; else data->hits++;

        return fault;
}

int LOG(Algorithm_Data *data)
{
        struct Frame *framep = data->page_table.lh_first, // lh_first = first element of queue 
                     *victim = NULL;

		struct Page_Log *page = NULL;
        int fault = 0;
		data->total_ref_count++;


		/* increase page reference count by 1 */
		int counted=0; 
		struct Page_Log *pg = NULL;
		TAILQ_FOREACH(pg, &data->page_ref_log, pages)
		{
			if(pg->page_num == last_page_ref)
			{
				pg->ref_count++;
				counted = 1;
				break;
			}
		}

		if(counted ==0) // if this page is never referenced before, add to log
		{
			page = malloc(sizeof(struct Page_Log));
			page->page_num = last_page_ref;
			page->ref_count = 1;
			TAILQ_INSERT_TAIL(&data->page_ref_log, page, pages);
			//data->page_ref_log_size++;
		}

		page = malloc(sizeof(struct Page_Log));
		page->page_num = last_page_ref;
		// insert to window log here
		TAILQ_INSERT_TAIL(&data->page_window_log, page, pages);
		data->page_ref_log_size++;

		if(_window_size > 0 && data->page_ref_log_size > _window_size)
		{
			page = data->page_window_log.tqh_first;
			TAILQ_FOREACH(pg, &data->page_ref_log, pages)
			{
				if(pg->page_num == page->page_num)
				{
					pg->ref_count--;
					break;
				}
			}
			if(page != NULL) // just in case 
			{
				TAILQ_REMOVE(&data->page_window_log, data->page_window_log.tqh_first, pages);
				data->page_ref_log_size--;
			}
		}

		/*
		 * search log list for ref_count of page num contained by frame list.
		 * pick the one with lowest hotness to evict. 
		 */
		double hotness = 0.0;
		double min_hotness=1.0;
		while(framep != NULL &&
				framep->page > -1 &&
				framep->page != last_page_ref)
		{

			if( _window_size > 0 && data->page_ref_log_size < _window_size)
			{
				if(victim==NULL || compare_time(framep->time, victim->time)==-1)
					victim = framep;
			}
			else
			{
				TAILQ_FOREACH(pg, &data->page_ref_log, pages)
				{
					if(framep->page == pg->page_num)
					{
						//fprintf(stderr, "%s:%d page %d hotness = %f\n", __FILE__, __LINE__, pg->page_num, (double)pg->ref_count/(double)data->total_ref_count);
						hotness = (double)pg->ref_count/(double)data->total_ref_count;
						if(hotness < min_hotness)
						{
							min_hotness = hotness;
							victim = framep;
						}
					}
				}
			}

            framep = framep->frames.le_next;
		}

        /* Make a decision */
        if(framep == NULL) 
        { // It's a miss, kill our victim, need to swap


			/*
			 * search swap list, if hit in swap, swap-in, else swap-out
			 * in this implementation, DAX is not supported. 
			 * victim is pointing to memory page to be evicted. 
			 * 1. add victim to victim list
			 * 1.2. check swap_list and swap-in if found in swap_list
			 * 1.3. if hit in swap, renew the reference time in swap.
			 * 1.5. if miss in swap, swap-out victim page to swap_list
			 * 2. change victim page's value to referenced page
			 */


			if(debug_flag) printf("Victim selected: %d, Page: %d\n", victim->index, victim->page);
			add_victim(&data->victim_list, victim);


			/*
			if(swap_in(&data->swap_list)==1) // last_page_ref is found in swap_list
				data->swap_in++;
			else
			{
				swap_out(&data->swap_list, victim);
				data->swap_out++;
			}
			*/

			victim->page = last_page_ref;
			clock_gettime(CLOCK_REALTIME,&victim->time );
			victim->extra = counter;
			fault = 1;


        }
        else if(framep->page == -1)
        { // Can use free page table index
                framep->page = last_page_ref;
				clock_gettime(CLOCK_REALTIME,&framep->time );
                framep->extra = counter;
                fault = 1;
        }
        else if(framep->page == last_page_ref)
        { // The page was found! Hit!
				clock_gettime(CLOCK_REALTIME,&framep->time );
                framep->extra = counter;
        }

		if(_window_size > 0)
		{
			if(data->total_ref_count >= _window_size && 
					(data->total_ref_count + _window_size) < max_page_calls )
			{
				if(fault == 1) data->misses++; else data->hits++;
			}
		}
		else
				if(fault == 1) data->misses++; else data->hits++;

        return fault;
}



/**
 * int LRU(Algorithm_Data *data)
 *
 * LRU Page Replacement Algorithm
 *
 * @param *data {Algorithm_Data} struct holding algorithm data
 *
 * return {int} did page fault, 0 or 1
 */
int LRU(Algorithm_Data *data)
{
        struct Frame *framep = data->page_table.lh_first, // lh_first = first element of queue 
                     *victim = NULL;

        int fault = 0;
		data->total_ref_count++;

		/*
		 *  search page table for the frame holding referenced page. 
		 */
        while (framep != NULL && 
				framep->page > -1 && 
				framep->page != last_page_ref) {

                if(victim == NULL || 
					compare_time(framep->time, victim->time)==-1)
				{
                        victim = framep; // No victim yet or frame older than victim
				}
                framep = framep->frames.le_next;

        }

        /* Make a decision */
        if(framep == NULL) 
        { // It's a miss, kill our victim, need to swap


			/*
			 * search swap list, if hit in swap, swap-in, else swap-out
			 * in this implementation, DAX is not supported. 
			 * victim is pointing to memory page to be evicted. 
			 * 1. add victim to victim list
			 * 1.2. check swap_list and swap-in if found in swap_list
			 * 1.3. if hit in swap, renew the reference time in swap.
			 * 1.5. if miss in swap, swap-out victim page to swap_list
			 * 2. change victim page's value to referenced page
			 */


			if(debug_flag) printf("Victim selected: %d, Page: %d\n", victim->index, victim->page);
			add_victim(&data->victim_list, victim);


			/*
			if(swap_in(&data->swap_list)==1) // last_page_ref is found in swap_list
				data->swap_in++;
			else
			{
				swap_out(&data->swap_list, victim);
				data->swap_out++;
			}
			*/

			victim->page = last_page_ref;
			clock_gettime(CLOCK_REALTIME,&victim->time );
			victim->extra = counter;
			fault = 1;


        }
        else if(framep->page == -1)
        { // Can use free page table index
                framep->page = last_page_ref;
				clock_gettime(CLOCK_REALTIME,&framep->time );
                framep->extra = counter;
                fault = 1;
        }
        else if(framep->page == last_page_ref)
        { // The page was found! Hit!
				clock_gettime(CLOCK_REALTIME,&framep->time );
                framep->extra = counter;
        }
		if(_window_size > 0)
		{
			if(data->total_ref_count >= _window_size && 
					(data->total_ref_count + _window_size) < max_page_calls )
			{
	        if(fault == 1) data->misses++; else data->hits++;
			}
		}
		else
	        if(fault == 1) data->misses++; else data->hits++;

        return fault;
}

/**
 * int CLOCK(Algorithm_Data *data)
 *
 * CLOCK Page Replacement Algorithm
 *
 * @param *data {Algorithm_Data} struct holding algorithm data
 *
 * return {int} did page fault, 0 or 1
 */
int CLOCK(Algorithm_Data *data)
{
        static Frame *clock_hand = NULL; // Clock needs a hand
        Frame *framep = data->page_table.lh_first;
        int fault = 0;
		data->total_ref_count++;
        /* Forward traversal. */
        /* Find target (hit), empty page slot (miss), or victim to evict (miss) */
        while(framep != NULL && framep->page > -1 && framep->page != last_page_ref)
                framep = framep->frames.le_next;
        /* Make a decision */
        if(framep != NULL)
        {
                if(framep->page == -1)
                {
                        framep->page = last_page_ref;
                        framep->extra = 0;
                        fault = 1;
                }
                else
                { // Found the page, update its R bit to 0
                        framep->extra = 0;
                }
        }
        else // Use the hand to find our victim
        {
                while(clock_hand == NULL || clock_hand->extra == 0)
                {
                        if(clock_hand == NULL)
                        {
                                clock_hand = data->page_table.lh_first;
                        }
                        else
                        {
                                clock_hand->extra = 1;
                                clock_hand = clock_hand->frames.le_next;
                        }
                }
                add_victim(&data->victim_list, clock_hand);
                clock_hand->page = last_page_ref;
                clock_hand->extra = 0;
                fault = 1;
        }
		if(_window_size>0)
		{
			if(data->total_ref_count >= _window_size && 
					(data->total_ref_count + _window_size) < max_page_calls )
			{
    	    if(fault == 1) data->misses++; else data->hits++;
			}
		}
		else
    	    if(fault == 1) data->misses++; else data->hits++;

        return fault;
}

/**
 * int NFU(Algorithm_Data *data)
 *
 * NFU Page Replacement Algorithm
 *
 * @param *data {Algorithm_Data} struct holding algorithm data
 *
 * return {int} did page fault, 0 or 1
 */
int NFU(Algorithm_Data *data)
{
        struct Frame *framep = data->page_table.lh_first,
                     *victim = NULL;
        int fault = 0;
		data->total_ref_count++;

        /* Find target (hit), empty page index (miss), or victim to evict (miss) */
        while (framep != NULL && framep->page > -1 && framep->page != last_page_ref) {
                if(victim == NULL || framep->extra < victim->extra)
                        victim = framep; // No victim or frame used fewer times
                framep = framep->frames.le_next;
        }
        /* Make a decision */
        if(framep == NULL)
        { // It's a miss, kill our victim
                add_victim(&data->victim_list, victim);
                victim->page = last_page_ref;
				clock_gettime(CLOCK_REALTIME,&victim->time );
                victim->extra = 0;
                fault = 1;
        }
        else if(framep->page == -1)
        { // Can use free page table index
                framep->page = last_page_ref;
				clock_gettime(CLOCK_REALTIME,&framep->time );
                framep->extra = 0;
                fault = 1;
        }
        else if(framep->page == last_page_ref)
        { // The page was found! Hit!
				clock_gettime(CLOCK_REALTIME,&framep->time );
                framep->extra++;
        }
		if(_window_size > 0)
		{
			if(data->total_ref_count >= _window_size && 
					(data->total_ref_count + _window_size) < max_page_calls )
			{
	        if(fault == 1) data->misses++; else data->hits++;
			}
		}
		else
	        if(fault == 1) data->misses++; else data->hits++;

        return fault;
}

/**
 * int AGING(Algorithm_Data *data)
 *
 * AGING Page Replacement Algorithm
 *
 * @param *data {Algorithm_Data} struct holding algorithm data
 *
 * return {int} did page fault, 0 or 1
 */
int AGING(Algorithm_Data *data)
{
        struct Frame *framep = data->page_table.lh_first,
                     *victim = NULL;
        int fault = 0;
		data->total_ref_count++;

        /* Find target (hit), empty page index (miss), or victim to evict (miss) */
        while (framep != NULL && framep->page > -1 && framep->page != last_page_ref) {
                framep->extra /= 2;
                if(victim == NULL || framep->extra < victim->extra)
                        victim = framep; // No victim or frame used rel less
                framep = framep->frames.le_next;
        }
        /* Make a decision */
        if(framep == NULL)
        { // It's a miss, kill our victim
                add_victim(&data->victim_list, victim);
                victim->page = last_page_ref;
                clock_gettime(CLOCK_REALTIME, &victim->time);
                victim->extra = 0;
                fault = 1;
        }
        else if(framep->page == -1)
        { // Can use free page table index
                framep->page = last_page_ref;
                clock_gettime(CLOCK_REALTIME, &framep->time);
                framep->extra = 0;
                fault = 1;
        }
        else if(framep->page == last_page_ref)
        { // The page was found! Hit!
                clock_gettime(CLOCK_REALTIME, &framep->time);
                framep->extra = framep->extra+10000000;
                while (framep->frames.le_next != NULL) {
                        framep = framep->frames.le_next;
                        framep->extra /= 2;
                }
        }

		if(_window_size > 0)
		{
			if(data->total_ref_count >= _window_size && 
					(data->total_ref_count + _window_size) < max_page_calls )
			{
    	    if(fault == 1) data->misses++; else data->hits++;
			}
		}
		else
    	    if(fault == 1) data->misses++; else data->hits++;

        return fault;
}

int LRU2(Algorithm_Data *data)
{
        struct Frame *framep = data->page_table.lh_first, // lh_first = first element of queue 
                     *victim = NULL;

        int fault = 0;
		int k_value = 2;
		struct Page_Log* page=NULL;

		/*
		 *  search page table for the frame holding referenced page. 
		 */
		int max_distance = 0;
		int tmp_d=0;
		int tmp_dk=0;
		int tmp_k=1;
		int use_lru=0;
//		fprintf(stderr, "current reference page = %d\n", last_page_ref);
		data->total_ref_count++;

			/*
			 * add to log
			 */
		page = malloc(sizeof(Page_Log));
		page->page_num = last_page_ref;
		TAILQ_INSERT_TAIL(&data->page_window_log, page, pages);

		struct Page_Log *pg = NULL;
        while (framep != NULL && 
				framep->page > -1 && 
				framep->page != last_page_ref) 
		{
			tmp_d = 0;
			tmp_dk= 0;
			tmp_k = 0;
			// run through log and find k-th furthest page
			TAILQ_FOREACH_REVERSE(pg, &data->page_window_log, Page_Win_List, pages)
			{
				tmp_d++;
//				fprintf(stderr, "%d ", pg->page_num);
				if(framep->page == pg->page_num)
				{
					tmp_k++;
					tmp_dk=tmp_d;
				}

				if(tmp_k==k_value)
				{
					use_lru=0;
					break;
				}
				else
					use_lru=1;
			}

			if(use_lru)
			{
				victim= NULL;
				break;
			}

			if(max_distance < tmp_dk)
			{
				max_distance = tmp_dk;
				victim = framep;
//				fprintf(stderr, "page %d distance %d \n", framep->page, max_distance);
			}

            framep = framep->frames.le_next;
        }


		// if victim is not found, use LRU
		if(victim==NULL)
		{
			//fprintf(stderr, "Using LRU in LRU2!!!\n");
			framep = data->page_table.lh_first;
    	    while (	framep != NULL && 
					framep->page > -1 && 
					framep->page != last_page_ref) 
			{
					if(victim == NULL ||
						compare_time(framep->time, victim->time)==-1)
					{
    	                    victim = framep; // No victim yet or frame older than victim
					}
            framep = framep->frames.le_next;
			}
		}


        /* Make a decision */
        if(framep == NULL) 
        { // It's a miss, kill our victim, need to swap

			if(debug_flag) printf("Victim selected: %d, Page: %d\n", victim->index, victim->page);
			add_victim(&data->victim_list, victim);

			victim->page = last_page_ref;
			clock_gettime(CLOCK_REALTIME,&victim->time );
			victim->extra = counter;
			fault = 1;


        }
        else if(framep->page == -1)
        { // Can use free page table index
                framep->page = last_page_ref;
				clock_gettime(CLOCK_REALTIME,&framep->time );
                framep->extra = counter;
                fault = 1;
        }
        else if(framep->page == last_page_ref)
        { // The page was found! Hit!
				clock_gettime(CLOCK_REALTIME,&framep->time );
                framep->extra = counter;
        }
		if(_window_size > 0)
		{
			if(data->total_ref_count >= _window_size && 
					(data->total_ref_count + _window_size) < max_page_calls )
			{
	        if(fault == 1) data->misses++; else data->hits++;
			}
		}
		else
	        if(fault == 1) data->misses++; else data->hits++;

        return fault;
}

int LRU3(Algorithm_Data *data)
{
        struct Frame *framep = data->page_table.lh_first, // lh_first = first element of queue 
                     *victim = NULL;

        int fault = 0;
		int k_value = 3;
		struct Page_Log* page=NULL;
		data->total_ref_count++;

			/*
			 * add to log
			 */
		page = malloc(sizeof(Page_Log));
		page->page_num = last_page_ref;
		TAILQ_INSERT_TAIL(&data->page_window_log, page, pages);

		/*
		 *  search page table for the frame holding referenced page. 
		 */
		int max_distance = 0;
		int tmp_d=0;
		int tmp_dk=0;
		int tmp_k=1;
		int use_lru = 0;
//		fprintf(stderr, "current reference page = %d\n", last_page_ref);
		struct Page_Log *pg = NULL;
        while (framep != NULL && 
				framep->page > -1 && 
				framep->page != last_page_ref) 
		{
			tmp_d = 0;
			tmp_dk= 0;
			tmp_k = 0;
			TAILQ_FOREACH_REVERSE(pg, &data->page_window_log, Page_Win_List, pages)
			{
				tmp_d++;
//				fprintf(stderr, "%d ", pg->page_num);
				if(framep->page == pg->page_num)
				{
					tmp_k++;
					tmp_dk=tmp_d;
				}

				if(tmp_k==k_value)
				{
					use_lru = 0;
					break;
				}
				else
					use_lru=1;

			}

			if(use_lru)
			{
				victim= NULL;
				break;
			}

//			fprintf(stderr, "\n");

			/*
			if(tmp_dk != tmp_d)
				tmp_dk = tmp_d;
				*/

			if(tmp_k > 1 && max_distance < tmp_dk)
			{
				max_distance = tmp_dk;
				victim = framep;
//				fprintf(stderr, "page %d distance %d \n", framep->page, max_distance);
			}

            framep = framep->frames.le_next;
        }



		// if victim is not found, use LRU
		if(victim==NULL)
		{
			framep = data->page_table.lh_first;
    	    while (	framep != NULL && 
					framep->page > -1 && 
					framep->page != last_page_ref) 
			{
					if(victim == NULL ||
						compare_time(framep->time, victim->time)==-1)
					{
    	                    victim = framep; // No victim yet or frame older than victim
					}
            framep = framep->frames.le_next;
			}
		}


        /* Make a decision */
        if(framep == NULL) 
        { // It's a miss, kill our victim, need to swap

			if(debug_flag) printf("Victim selected: %d, Page: %d\n", victim->index, victim->page);
			add_victim(&data->victim_list, victim);

			victim->page = last_page_ref;
			clock_gettime(CLOCK_REALTIME,&victim->time );
			victim->extra = counter;
			fault = 1;


        }
        else if(framep->page == -1)
        { // Can use free page table index
                framep->page = last_page_ref;
				clock_gettime(CLOCK_REALTIME,&framep->time );
                framep->extra = counter;
                fault = 1;
        }
        else if(framep->page == last_page_ref)
        { // The page was found! Hit!
				clock_gettime(CLOCK_REALTIME,&framep->time );
                framep->extra = counter;
        }
		if(_window_size > 0)
		{
			if(data->total_ref_count >= _window_size && 
					(data->total_ref_count + _window_size) < max_page_calls )
			{
	        if(fault == 1) data->misses++; else data->hits++;
			}
		}
		else
	        if(fault == 1) data->misses++; else data->hits++;

        return fault;
}



/**
 * int print_help()
 *
 * @param num_frames {int} number of page frames in simulated page table
 *
 * Function to print results after algo is ran
 */
void print_help(const char *binary)
{
        printf( "usage: %s -a [algorithm] -f [num_frames] -s -v  \n", binary);
        printf( "   -a algorithm    - page algorithm to use, e.g., \"LRU,CLOCK\"\n");
        printf( "   -f num_frames   - number of page frames {int > 0}\n");
        printf( "   -v - print page table after each ref is processed {1 or 0}\n");
        printf( "   -d - verbose debugging output {1 or 0}\n");
        printf( "   -r - verbose debugging output {1 or 0}\n");
        printf( "   -d - verbose debugging output {1 or 0}\n");
		exit(0);
}

/**
 * int print_stats()
 *
 * Function to print results after algo is ran
 */
int print_stats(Algorithm algo)
{
        print_summary(algo);
        print_list(algo.data->page_table.lh_first, "Frame #", "Page Ref");
        return 0;
}

/**
 * int print_summary()
 *
 * Function to print summary report of an Algorithm
 */
int print_summary(Algorithm algo)
{
        printf("%s Algorithm\n", algo.label);
        printf("Frames in Mem: %d, ", num_frames);
        printf("Hits: %d, ", algo.data->hits);
        printf("Misses: %d, ", algo.data->misses);
		/*
        printf("Swap out: %zu, ", algo.data->swap_out);
        printf("Swap in: %zu, ", algo.data->swap_in);
        printf("Swap I/O: %zu, ", algo.data->swap_out + algo.data->swap_in);
		*/
        printf("Hit Ratio: %f\n", (double)algo.data->hits/(double)(algo.data->hits+algo.data->misses));
//		printf("swap on HDD takes %f mu-seconds\n", (double) (HDD_READ_LATENCY * algo.data->swap_in + HDD_WRITE_LATENCY * algo.data->swap_out));
//		printf("swap on SSD takes %f mu-seconds\n", (double) (SSD_READ_LATENCY * algo.data->swap_in + SSD_WRITE_LATENCY * algo.data->swap_out));
//		printf("swap on PCM takes %f mu-seconds\n", (double) (1000*PCM_READ_LATENCY * (double)algo.data->swap_in + 1000*PCM_WRITE_LATENCY * (double)algo.data->swap_out)/1000);

        return 0;
}

/**
 * int print_list()
 *
 * Print list
 *
 * @param head {Frame} head of frame list
 * @param index_label {const char*} label for index frame field
 * @param value_label {const char*} label for value frame field
 *
 * @retun 0
 */
int print_list(struct Frame *head, const char* index_label, const char* value_label)
{
        int colsize = 9, labelsize;
        struct Frame *framep;
        // Determine lanbel col size from text
        if (strlen(value_label) > strlen(index_label))
                labelsize = strlen(value_label) + 1;
        else
                labelsize = strlen(index_label) + 1;
        /* Forward traversal. */
        printf("%-*s: ", labelsize, index_label);
        for (framep = head; framep != NULL; framep = framep->frames.le_next)
        {
                printf("%*d", colsize, framep->index);
        }
        printf("\n%-*s: ", labelsize, value_label);
        for (framep = head; framep != NULL; framep = framep->frames.le_next)
        {
                if(framep->page == -1)
                        printf("%*s", colsize, "_");
                else
                        printf("%*d", colsize, framep->page);
        }
        printf("\n%-*s: ", labelsize, "Extra");
        for (framep = head; framep != NULL; framep = framep->frames.le_next)
        {
                printf("%*d", colsize, framep->extra);
        }
        printf("\n%-*s: ", labelsize, "Time");
        for (framep = head; framep != NULL; framep = framep->frames.le_next)
        {
                printf("% 2d.%*lu", (int) (framep->time.tv_sec - _start_time), colsize-3, (unsigned long) (framep->time.tv_nsec/1000));
        }
        printf("\n\n");

        return 0;
}

/**
 * int cleanup()
 *
 * Clean up memory
 *
 * @return 0
 */
int cleanup()
{

	if(_fp != NULL)
		fclose(_fp);

        size_t i = 0;
        for (i = 0; i < num_algos; i++)
        {
                /* Clean up memory, delete the list */
                while (algos[i].data->page_ref_log.tqh_first != NULL)
                        TAILQ_REMOVE(&algos[i].data->page_ref_log, algos[i].data->page_ref_log.tqh_first, pages);
                while (algos[i].data->page_window_log.tqh_first != NULL)
                        TAILQ_REMOVE(&algos[i].data->page_window_log, algos[i].data->page_window_log.tqh_first, pages);
                while (algos[i].data->page_table.lh_first != NULL)
                        LIST_REMOVE(algos[i].data->page_table.lh_first, frames);
                while (algos[i].data->victim_list.lh_first != NULL)
                        LIST_REMOVE(algos[i].data->victim_list.lh_first, frames);
                while (algos[i].data->swap_list.lh_first != NULL)
                        LIST_REMOVE(algos[i].data->swap_list.lh_first, frames);
        }
        return 0;
}
