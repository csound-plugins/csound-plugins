import numpy as np
import matplotlib.pyplot as plt
from scipy import signal
import sndfileio

times = np.load("pyin-times.npy")
freqs = np.load("pyin-freq.npy")
confidence = np.load("pyin-confidence.npy")
env = np.load("pyin-env.npy")
samples, sr = sndfileio.sndread("pyin-test.wav")

unvoiced = env < 0.01
freqs[unvoiced] = float("nan")

figsize = (24, 8)
fftsize = 2048
overlap = 8
window = 'hamming'
cmap = "inferno"
mindb = -90
interpolation = "bilinear"
minfreq = 40
maxfreq = 1000

winsize = fftsize
f = plt.figure(figsize=figsize)
axes = f.add_subplot(1, 1, 1)

hopsize = int(fftsize // overlap)
noverlap = fftsize - hopsize

win = signal.get_window(window, winsize)
axes.specgram(samples[:, 0],
              NFFT=fftsize,
              Fs=sr,
              noverlap=noverlap,
              window=win,
              cmap=cmap,
              interpolation=interpolation,
              vmin=mindb)
axes.set_ylim(minfreq, maxfreq)
axes.xaxis.set_label_text('Time')
axes.yaxis.set_label_text('Hz')


axes.plot(times, freqs)

f.savefig("pyin-spectrogram.png")
