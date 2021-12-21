## Creating your own ringtone

Fastest way to create your own ringtone is using ffmpeg to resample the tone you want to make into what openqti will understand:

ffmpeg -i [ORIGINAL FILE] -ac 1 -ar 8000 -acodec pcm_s16le ring8k.wav

After converting, place your file either at /tmp/ for runtime only testing or to /usr/share/tones to make it permanent