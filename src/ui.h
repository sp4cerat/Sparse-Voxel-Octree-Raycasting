#include <nvwidgets/nvGlutWidgets.h>

enum UIOption {
    OPTION_DIFF,
    OPTION_DXT5_YCOCG,
    OPTION_COMPRESS,
    OPTION_COMPRESS2,
    OPTION_ANIMATE,
    OPTION_COUNT
};


static bool options[OPTION_COUNT];
static nv::GlutUIContext ui;
static int win_w = 512, win_h = 512;
static float musicVolume = 128.0f;
static float compressionRate = 0.0f;

static void doUI()
{
	//Mix_VolumeMusic(int(musicVolume));

    static nv::Rect none;
    
    ui.begin();

	static nv::Rect r=nv::Rect(200,200,0,0);

	static bool unfold[5]={true,true,true};
	
	//int tid=0;
	//ui.doTextureView(r2,&tid,r);
    //ui.beginGroup(nv::GroupFlags_GrowDownFromLeft,r );
	char title[100];
	sprintf(title,"Fps %f Fps CL: %f",fps,fpscl);

	ui.beginPanel(r,title,unfold);
	if(unfold[0]==true)
	{
		{
			static char  text[100]={"text1"};
			static int   textlen=0;
			ui.doLineEdit(none,&text[0],20,&textlen);
		}
		{
			static char  text[100]={"text2"};
			static int   textlen=0;
			ui.doLineEdit(none,&text[0],20,&textlen);
		}
		ui.doCheckButton(none, "Enable compression", &options[OPTION_COMPRESS]);

		ui.beginGroup( nv::GroupFlags_GrowLeftFromTop);
		{
			char  text[100];
			ui.doHorizontalSlider(none, 1920/4,1920, &raycast_width);
			sprintf(text,"%3.0f",raycast_width);	ui.doLabel( none, text);
		}
		ui.endGroup();
		ui.beginGroup( nv::GroupFlags_GrowLeftFromTop);
		{
			char  text[100];
			ui.doHorizontalSlider(none, 1080/4,1080, &raycast_height);
			sprintf(text,"%3.0f",raycast_height);ui.doLabel( none, text);
		}
		ui.endGroup();
		

		if (options[OPTION_COMPRESS])
		{
			ui.beginGroup(nv::GroupFlags_GrowLeftFromTop|nv::GroupFlags_LayoutNoMargin);
                
				ui.doCheckButton(none, "Show difference", &options[OPTION_DIFF]);
                
				if (options[OPTION_DIFF])
				{
					ui.doHorizontalSlider(none, 1.0f,128.0f, &musicVolume);
				}

			ui.endGroup();
        
			ui.beginGroup( nv::GroupFlags_GrowLeftFromTop);

				ui.doLabel( none, "Format");

				const char * formatLabel[] = { "YCoCg-DXT5", "DXT1" };
				int formatIdx = options[OPTION_DXT5_YCOCG] ? 0 : 1;
				ui.doComboBox(none, 2, formatLabel, &formatIdx);
				options[OPTION_DXT5_YCOCG] = (formatIdx == 0);
        
			ui.endGroup();
		}
	}
	ui.endPanel();
   // ui.endGroup();

    if (options[OPTION_COMPRESS])
    {
        ui.beginGroup(nv::GroupFlags_GrowDownFromRight);

            if (ui.doButton(none, "Benchmark"))
            {
                // doBenchmark = true;
            }

           // if (compressionRate != 0.0f)
            {
                char text[256];
                sprintf(text, "%.2f Mpixels/sec", 100);
                ui.doLabel(none, text);
            }

        ui.endGroup();
    }

    // Pass non-ui mouse events to the manipulator
	//updateManipulator(ui, manipulator);

    ui.end();
}

