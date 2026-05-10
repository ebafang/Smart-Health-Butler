#include "common.h"

#define REC_CMD    "arecord -d3 -c1 -r16000 -traw -fS16_LE cmd.pcm"
#define PCM_FILE   "./cmd.pcm"
#define DEV_PATH2  "/dev/ttySAC2"  // 串口2的驱动路径

void showbitmap(bitmap *bm, int x0, int y0);
void font_show(char *text, int pixelSize, int outFrameWidth, int outFrameHeight,
               int outFrameColor, int fontPosX, int fontPosY, int fontColor,
               int frameToLcdPosX, int frameToLcdPosY);
int get_voice_info(int sockfd);
void serial_init(void);
int get_stm32_data(char *stm_cmd);
int get_stm32_mksall(int *hr, int *sbp, int *dbp, int *spo2);
float get_stm32_temp(void);

extern unsigned char *lcd;
int  tty2_fd;

// ==================== 启动界面 ====================
void start(void)
{
	int tx, ty;
	// 显示启动界面
	show_bmp("/ai_bmp/start.bmp", 0, 0);
	font_show("亳院二队：方正，刘奥，关灿",
						   32, 700, 300, 0xffffff00, 10, 80, 0xff000000, 36, 26);
	// 播放欢迎语音（后台播放不阻塞）
	system("aplay /ai_wav/welcome.wav&");
	// 检测启动按钮
	while (1)
	{
		get_xy(&tx, &ty);
		if (tx > 330 && tx <470  && ty > 158 && ty < 300)
		{
			break;
		}
	}
}

// ==================== AI语音助手界面 ====================
void ai_voice(int sockfd)
{
	int tx, ty;           // 触屏坐标
	int n = 0;            // 获取的数据
	char font_data[1024] = {0}; // 屏幕显示文字

	// 显示AI界面
	show_bmp("ai_bmp/voice.bmp", 0, 0);

	// 检测语音按钮
	while (1)
	{
		get_xy(&tx, &ty);
		if (tx > 360 && tx < 445 && ty > 215 && ty < 355)
		{
			int cmd = get_voice_info(sockfd);
			printf("Voice CMD ID: %d\n", cmd);

			if (cmd == 0)
			{
				printf("help menu\n");
				show_bmp("ai_bmp/voice.bmp", 0, 0);
				font_show("语音指令大全", 38, 330, 50, 0xffffff00, 80, 5, 0xff0000ff, 10, 10);
				font_show("\"你好\" -- 打招呼", 28, 150, 50, 0xff00ff00, 10, 5, 0xff000000, 10, 60);
				font_show("\"量血压\" -- 测血压(收缩压/舒张压)", 28, 150, 50, 0xff00ff00, 10, 5, 0xff000000, 170, 60);
				font_show("\"测心率\" -- 测心率", 28, 150, 50, 0xff00ff00, 10, 5, 0xff000000, 10, 110);
				font_show("\"测血氧\" -- 测血氧", 28, 150, 50, 0xff00ff00, 10, 5, 0xff000000, 170, 110);
				font_show("\"测体温\" -- 测体温", 28, 150, 50, 0xff00ff00, 10, 5, 0xff000000, 10, 160);
				font_show("\"健康报告\" -- 综合检测", 28, 150, 50, 0xff00ff00, 10, 5, 0xff000000, 170, 160);
				font_show("\"你是谁\" -- 自我介绍", 28, 150, 50, 0xff00ff00, 10, 5, 0xff000000, 10, 210);
				font_show("\"再见\" -- 退出系统", 28, 150, 50, 0xff00ff00, 10, 5, 0xff000000, 170, 210);
				font_show("请说出您需要的功能", 32, 330, 50, 0xffffff00, 30, 10, 0xff0000ff, 10, 310);
				system("aplay /ai_wav/help.wav&");
				sleep(5);
			}
			if (cmd == 2) // 打招呼
			{
				printf("hello\n");
				show_bmp("ai_bmp/voice.bmp", 0, 0);
				font_show("你好！我是智能健康管家",
						   32, 700, 300, 0xffffff00, 10, 80, 0xff000000, 36, 26);
				system("aplay /ai_wav/nihao.wav&");
				sleep(3);
			}
			if (cmd == 3) // 量血压（收缩压+舒张压，一次充气获取）
			{
				int sbp = 0, dbp = 0, hr = 0, spo2 = 0;

				show_bmp("ai_bmp/voice.bmp", 0, 0);
				font_show("请把手放到传感器，等待检测...",
						   32, 700, 300, 0xffffff00, 10, 80, 0xff000000, 36, 26);
				system("aplay /ai_wav/dengdai.wav");

				get_stm32_mksall(&hr, &sbp, &dbp, &spo2);

				bzero(font_data, 1024);
				if (sbp > 0 && dbp > 0)
				{
					if (sbp < 120 && dbp < 80)
						sprintf(font_data, "收缩压：%d  舒张压：%d  血压理想，继续保持！", sbp, dbp);
					else if (sbp < 130 && dbp < 85)
						sprintf(font_data, "收缩压：%d  舒张压：%d  血压正常高值，注意保持！", sbp, dbp);
					else if (sbp < 140 || dbp < 90)
						sprintf(font_data, "收缩压：%d  舒张压：%d  血压偏高，请注意饮食和运动", sbp, dbp);
					else if (sbp < 160 || dbp < 100)
						sprintf(font_data, "收缩压：%d  舒张压：%d  一级高血压，建议就医检查", sbp, dbp);
					else if (sbp < 180 || dbp < 110)
						sprintf(font_data, "收缩压：%d  舒张压：%d  二级高血压，请尽快就医", sbp, dbp);
					else
						sprintf(font_data, "收缩压：%d  舒张压：%d  三级高血压，危险！请立即就医！", sbp, dbp);

					if (sbp >= 140 || dbp >= 90)
						system("aplay /ai_wav/result_high.wav");
					else if (sbp <= 90 || dbp <= 60)
						system("aplay /ai_wav/result_low.wav");
					else
						system("aplay /ai_wav/result_normal.wav");
				}
				else
				{
					sprintf(font_data, "血压测量失败，请检查传感器连接");
				}

				printf("BP result: %s\n", font_data);
				show_bmp("ai_bmp/voice.bmp", 0, 0);
				font_show(font_data, 32, 700, 300, 0xffffff00, 10, 80, 0xff000000, 36, 26);
				sleep(5);
			}
			if (cmd == 6) // 测心率
			{
				show_bmp("ai_bmp/voice.bmp", 0, 0);
				font_show("请把手放到传感器，等待检测...",
						   32, 700, 300, 0xffffff00, 10, 80, 0xff000000, 36, 26);
				system("aplay /ai_wav/dengdai.wav");

				n = get_stm32_data("mks-hr\n");

				bzero(font_data, 1024);
				if (n > 0)
				{
					if (n >= 60 && n <= 100)
						sprintf(font_data, "心率：%d  心率正常，状态良好", n);
					else if (n < 60 && n >= 40)
						sprintf(font_data, "心率：%d  心率偏低，建议观察", n);
					else if (n < 40)
						sprintf(font_data, "心率：%d  心率过缓，请就医检查", n);
					else if (n <= 120)
						sprintf(font_data, "心率：%d  心率偏快，建议休息后复测", n);
					else
						sprintf(font_data, "心率：%d  心率过快，请就医检查", n);
				}
				else
				{
					sprintf(font_data, "心率测量失败，请检查传感器连接");
				}

				printf("HR result: %s\n", font_data);
				show_bmp("ai_bmp/voice.bmp", 0, 0);
				font_show(font_data, 32, 700, 300, 0xffffff00, 10, 80, 0xff000000, 36, 26);
				if (n > 0)
				{
					if (n > 100)
						system("aplay /ai_wav/hr_high.wav");
					else if (n < 60)
						system("aplay /ai_wav/hr_low.wav");
					else
						system("aplay /ai_wav/result_normal.wav");
				}
				sleep(3);
			}
			if (cmd == 7) // 测血氧
			{
				show_bmp("ai_bmp/voice.bmp", 0, 0);
				font_show("请把手放到传感器，等待检测...",
						   32, 700, 300, 0xffffff00, 10, 80, 0xff000000, 36, 26);
				system("aplay /ai_wav/dengdai.wav");

				n = get_stm32_data("mks-spo2\n");

				bzero(font_data, 1024);
				if (n > 0)
				{
					if (n >= 98)
						sprintf(font_data, "血氧饱和度：%d%%  血氧饱和度极佳", n);
					else if (n >= 95)
						sprintf(font_data, "血氧饱和度：%d%%  血氧饱和度正常", n);
					else if (n >= 90)
						sprintf(font_data, "血氧饱和度：%d%%  血氧偏低，建议吸氧观察", n);
					else
						sprintf(font_data, "血氧饱和度：%d%%  血氧危险！请立即就医！", n);
				}
				else
				{
					sprintf(font_data, "血氧测量失败，请检查传感器连接");
				}

				printf("SPO2 result: %s\n", font_data);
				show_bmp("ai_bmp/voice.bmp", 0, 0);
				font_show(font_data, 32, 700, 300, 0xffffff00, 10, 80, 0xff000000, 36, 26);
				if (n > 0)
				{
					if (n < 90)
						system("aplay /ai_wav/result_low.wav");
					else
						system("aplay /ai_wav/result_normal.wav");
				}
				sleep(3);
			}
			if (cmd == 8) // 测体温
			{
				show_bmp("ai_bmp/voice.bmp", 0, 0);
				font_show("请稍候，正在测量体温...",
						   32, 700, 300, 0xffffff00, 10, 80, 0xff000000, 36, 26);
				system("aplay /ai_wav/dengdai.wav");

				float temp = get_stm32_temp();

				bzero(font_data, 1024);
				if (temp > 0)
				{
					if (temp >= 36.0 && temp <= 37.3)
						sprintf(font_data, "体温：%.1f  体温正常，状态良好", temp);
					else if (temp < 35.0)
						sprintf(font_data, "体温：%.1f  体温过低，请注意保暖", temp);
					else if (temp < 36.0)
						sprintf(font_data, "体温：%.1f  体温偏低，建议多喝热水", temp);
					else if (temp <= 38.0)
						sprintf(font_data, "体温：%.1f  低烧，请注意休息", temp);
					else if (temp <= 39.0)
						sprintf(font_data, "体温：%.1f  中度发烧，建议就医", temp);
					else
						sprintf(font_data, "体温：%.1f  高烧！请立即就医！", temp);
				}
				else
				{
					sprintf(font_data, "体温测量失败，请检查传感器连接");
				}

				printf("TEMP result: %s\n", font_data);
				show_bmp("ai_bmp/voice.bmp", 0, 0);
				font_show(font_data, 32, 700, 300, 0xffffff00, 10, 80, 0xff000000, 36, 26);
				if (temp > 0)
				{
					if (temp > 37.3)
						system("aplay /ai_wav/result_high.wav");
					else if (temp < 36.0)
						system("aplay /ai_wav/result_low.wav");
					else
						system("aplay /ai_wav/result_normal.wav");
				}
				sleep(3);
			}
			if (cmd == 10) // 综合健康报告（一次充气获取血压+心率+血氧）
			{
				int sbp = 0, dbp = 0, hr = 0, spo2 = 0;
				float temp = 0;

				show_bmp("ai_bmp/voice.bmp", 0, 0);
				font_show("正在进行综合健康检测，请稍候...",
						   32, 700, 300, 0xffffff00, 10, 80, 0xff000000, 36, 26);
				system("aplay /ai_wav/please_wait.wav&");

				// 一次充气获取血压+心率+血氧
				get_stm32_mksall(&hr, &sbp, &dbp, &spo2);
				printf("SBP: %d, DBP: %d, HR: %d, SPO2: %d\n", sbp, dbp, hr, spo2);

				// 体温
				temp = get_stm32_temp();
				printf("TEMP: %.1f\n", temp);

				bzero(font_data, 1024);
				snprintf(font_data, sizeof(font_data),
					"==== 健康报告 ====\n"
					"收缩压：%d mmHg\n"
					"舒张压：%d mmHg\n"
					"心率：%d 次/分\n"
					"血氧：%d%%\n"
					"体温：%.1f C\n"
					"================",
					sbp, dbp, hr, spo2, temp);

				show_bmp("ai_bmp/voice.bmp", 0, 0);
				font_show(font_data, 24, 700, 350, 0xffffff00, 10, 10, 0xff000000, 36, 26);
				system("aplay /ai_wav/result_normal.wav");
				sleep(5);
			}
			if (cmd == 66) // 自我介绍
			{
				show_bmp("ai_bmp/voice.bmp", 0, 0);
				bzero(font_data, 1024);
				sprintf(font_data,
					"我是智能健康管家！我可以帮你测量血压、心率、血氧和体温，并给出健康建议。请说帮助查看所有功能。");
				font_show(font_data, 26, 700, 300, 0xffffff00, 10, 10, 0xff000000, 36, 26);
				system("aplay /ai_wav/who.wav&");
				sleep(3);
			}
			if (cmd == 999) // 退出
			{
				show_bmp("ai_bmp/voice.bmp", 0, 0);
				font_show("感谢使用！再见！", 40, 430, 147, 0x00000000, 120, 45, 0xff0000ff, 200, 58);
				system("aplay /ai_wav/thank_you.wav");
				sleep(2);
				break;
			}
			if (cmd == -1) // 触屏取消或语音超时，静默恢复
			{
				printf("Voice cancelled/timeout, back to idle.\n");
			}
			else if (cmd < 0) // 未识别
			{
				font_show("我没听清楚，请再说一遍。说帮助查看所有功能。",
						   28, 700, 300, 0xffffff00, 10, 80, 0xff000000, 36, 26);
			}

			// 返回语音界面
			sleep(1);
			show_bmp("ai_bmp/voice.bmp", 0, 0);
		}
	}
}

// ==================== 主函数 ====================
int main(int argc, char const *argv[])
{
	// 参数检测
	if (argc != 2)
	{
		printf("Usage: %s <ubuntu-IP>\n", argv[0]);
		exit(0);
	}

	// 硬件初始化
	lcd_init();     // 显示屏
	ts_init();      // 触摸屏
	serial_init();  // 串口--stm32

	// 与语音识别引擎建立连接
	int sockfd = init_sock(argv[1]);

	// 启动界面
	start();
	printf("waiting touch...\n");

aa:
	// AI语音界面
	ai_voice(sockfd);
	goto aa;

	close(sockfd);
	return 0;
}

// ==================== STM32一次充气获取HR,SBP,DBP,SPO2 ====================
int get_stm32_mksall(int *hr, int *sbp, int *dbp, int *spo2)
{
	char stm_data[1024] = {0};
	int timeout = 90, n, _hr = 0, _sbp = 0, _dbp = 0, _spo2 = 0;

	printf("send cmd: mks-all (timeout=%ds)\n", timeout);
	tcflush(tty2_fd, TCIFLUSH);
	write(tty2_fd, "mks-all\n", 8);

	while (timeout > 0)
	{
		sleep(1);
		bzero(stm_data, sizeof(stm_data));
		n = read(tty2_fd, stm_data, sizeof(stm_data) - 1);
		if (n > 0)
		{
			stm_data[n] = '\0';
			printf("STM32 raw reply: \"%s\" (%d bytes)\n", stm_data, n);

			char *nl = strchr(stm_data, '\n');
			if (nl) *nl = '\0';
			nl = strchr(stm_data, '\r');
			if (nl) *nl = '\0';

			if (sscanf(stm_data, "%d,%d,%d,%d", &_hr, &_sbp, &_dbp, &_spo2) >= 4)
			{
				if (_hr > 0 && _sbp > 0)
				{
					*hr = _hr; *sbp = _sbp; *dbp = _dbp; *spo2 = _spo2;
					return 1;
				}
			}
			/* 传感器还没出数，重新发命令 */
			printf("STM32 returned 0, resending cmd...\n");
			tcflush(tty2_fd, TCIFLUSH);
			write(tty2_fd, "mks-all\n", 8);
		}
		timeout--;
	}

	printf("STM32 mks-all timeout\n");
	return 0;
}

// ==================== STM32单值通信函数 ====================
int get_stm32_data(char *stm_cmd)
{
	char stm_data[1024] = {0};
	int timeout, n;

	if (strstr(stm_cmd, "mks-hr") || strstr(stm_cmd, "mks-spo2"))
		timeout = 60;
	else if (strstr(stm_cmd, "asm"))
		timeout = 50;
	else
		return 0;

	printf("send cmd: %s (timeout=%ds)\n", stm_cmd, timeout);
	tcflush(tty2_fd, TCIFLUSH);

	int len = strlen(stm_cmd);
	write(tty2_fd, stm_cmd, len);

	while (timeout > 0)
	{
		sleep(1);
		bzero(stm_data, sizeof(stm_data));
		n = read(tty2_fd, stm_data, sizeof(stm_data) - 1);
		if (n > 0)
		{
			stm_data[n] = '\0';
			printf("STM32 raw reply: \"%s\" (%d bytes)\n", stm_data, n);

			char *nl = strchr(stm_data, '\n');
			if (nl) *nl = '\0';
			nl = strchr(stm_data, '\r');
			if (nl) *nl = '\0';

			int val = atoi(stm_data);
			if (val > 0)
				return val;
			/* STM32返回0表示传感器还没出数，重新发命令触发一次新测量 */
			printf("STM32 returned 0, resending cmd...\n");
			tcflush(tty2_fd, TCIFLUSH);
			write(tty2_fd, stm_cmd, len);
		}
		timeout--;
	}

	printf("STM32 timeout, no data received\n");
	return 0;
}

// ==================== STM32体温获取（返回浮点℃） ====================
float get_stm32_temp(void)
{
	char stm_data[1024] = {0};
	int timeout = 50, n;

	printf("send cmd: asm-temp (timeout=%ds)\n", timeout);
	tcflush(tty2_fd, TCIFLUSH);
	write(tty2_fd, "asm-temp\n", 9);

	while (timeout > 0)
	{
		sleep(1);
		bzero(stm_data, sizeof(stm_data));
		n = read(tty2_fd, stm_data, sizeof(stm_data) - 1);
		if (n > 0)
		{
			stm_data[n] = '\0';
			printf("STM32 raw reply: \"%s\" (%d bytes)\n", stm_data, n);

			char *nl = strchr(stm_data, '\n');
			if (nl) *nl = '\0';
			nl = strchr(stm_data, '\r');
			if (nl) *nl = '\0';

			float val = atof(stm_data);
			if (val > 30.0)
				return val;
			/* STM32返回0表示温度传感器还没接触，重新发命令触发一次新测量 */
			printf("STM32 temp returned %.1f, resending cmd...\n", val);
			tcflush(tty2_fd, TCIFLUSH);
			write(tty2_fd, "asm-temp\n", 9);
		}
		timeout--;
	}

	printf("STM32 temp timeout\n");
	return 0;
}

// ==================== 硬件初始化 ====================
void serial_init(void)
{
	tty2_fd = open(DEV_PATH2, O_RDWR | O_NOCTTY);
	init_tty(tty2_fd);
}

// ==================== 语音识别 ====================
int get_voice_info(int sockfd)
{
	int id_num;

	show_bmp("ai_bmp/voice.bmp", 0, 0);
	font_show("我在听，请说出你的需求",
			   32, 700, 300, 0xffffff00, 10, 80, 0xff000000, 36, 26);

	system(REC_CMD);
	send_pcm(sockfd, PCM_FILE);

	xmlChar *id = wait4id(sockfd);
	if (id == NULL)
		return -1;

	id_num = atoi((char *)id);
	printf("id: %d\n", id_num);
	return id_num;
}

// ==================== 底层显示函数 ====================
void showbitmap(bitmap *bm, int x0, int y0)
{
	int x, y;
	int bgrIndex;
	unsigned char alpha;
	for (y = y0; y < bm->height + y0; y++)
	{
		for (x = x0; x < bm->width + x0; x++)
		{
			bgrIndex = ((y - y0) * bm->width + (x - x0)) * 4;
			alpha = bm->map[bgrIndex + 3];
			if (alpha == 0)
				continue;
			lcd[y * 800 * 4 + x * 4 + 0] = bm->map[bgrIndex + 0]; // B
			lcd[y * 800 * 4 + x * 4 + 1] = bm->map[bgrIndex + 1]; // G
			lcd[y * 800 * 4 + x * 4 + 2] = bm->map[bgrIndex + 2]; // R
			lcd[y * 800 * 4 + x * 4 + 3] = bm->map[bgrIndex + 3]; // A
		}
	}
}

void font_show(char *text, int pixelSize, int outFrameWidth, int outFrameHeight,
               int outFrameColor, int fontPosX, int fontPosY, int fontColor,
               int frameToLcdPosX, int frameToLcdPosY)
{
	font *f = fontLoad("/simfang.ttf");
	fontSetSize(f, pixelSize);
	bitmap *bm = createBitmapWithInit(outFrameWidth, outFrameHeight, 4, outFrameColor);
	fontPrint(f, bm, fontPosX, fontPosY, text, fontColor, 0);
	showbitmap(bm, frameToLcdPosX, frameToLcdPosY);
	destroyBitmap(bm);
	fontUnload(f);
}
