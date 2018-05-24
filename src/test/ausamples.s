	.data
	.global snd_click
	.global snd_click_size
snd_click:
	.incbin "click-s8_mono.pcm"
snd_click_size: .long . - snd_click
