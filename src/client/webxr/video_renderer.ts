import { MediaInterface } from './media_interface';
import { SbsRendererBase } from './sbs_renderer_base';
import { VRInput } from './vr_input';
import * as mediaUtils from '../modules/media_utils';
import * as THREE from 'three';
import * as types from '../modules/client_types';

// Renderer for left/right eye  videos.
export class VideoRenderer extends SbsRendererBase {
  private mediaInterface: MediaInterface;

  // The video element and texture to play back video media.
  private videoElement?: HTMLVideoElement;
  private videoTexture?: THREE.VideoTexture;
  private hasActiveMedia = false;

  // In seek mode, left and right directions seek by 10 seconds.
  private seekMode = false;
  private hasSeekDir = false;

  constructor(
    renderer: THREE.WebGLRenderer,
    scene: THREE.Scene,
    vrInput: VRInput,
    mediaInterface: MediaInterface,
  ) {
    super(renderer, scene, vrInput);
    this.mediaInterface = mediaInterface;

    // Register media callbacks.
    this.mediaInterface.addMediaChangedCallback(() => {
      this.changeMedia(this.mediaInterface.getMedia());
    });
    this.mediaInterface.addVideoPauseCallback(() => {
      const videoState = this.mediaInterface.getVideoState();
      if (!videoState) return;
      this.setVideoPause(videoState.paused);
    });
    this.mediaInterface.addVideoMuteCallback(() => {
      const videoState = this.mediaInterface.getVideoState();
      if (!videoState) return;
      this.setVideoMute(videoState.muted);
    });
  }

  override cleanupResources() {
    // Dispose the master video texture.
    this.videoTexture?.dispose();
    this.videoTexture = undefined;

    // Clean up the old video element.
    if (this.videoElement) {
      this.videoElement.pause();
      this.videoElement.src = '';
      this.videoElement.load();
      this.videoElement = undefined;
    }

    this.hasActiveMedia = false;
    super.cleanupResources();
  }

  updateFrame() {
    this.processInputs();
    this.updateVideoState();
  }

  // TODO: This should be removed. Seeking can be done via mediaInterface.
  setSeekMode(seekMode: boolean) {
    this.seekMode = seekMode;
  }

  private changeMedia(media?: types.SelectedMedia) {
    this.cleanupResources();
    if (!media || !mediaUtils.isVideoMedia(media)) return;

    // Load the video and assign the left/right textures.
    this.videoElement = document.createElement('video');
    this.videoElement.src = mediaUtils.getMediaUrl(media);
    this.videoElement.crossOrigin = 'anonymous';
    this.videoElement.loop = true;
    this.videoElement.muted = true;
    this.videoElement.playsInline = true;
    this.videoElement.play().then(() => {
      if (!this.videoElement) return;
      this.videoTexture = new THREE.VideoTexture(this.videoElement);
      this.configureVideoTexture(this.videoTexture);
      this.setSideBySideTexture(this.videoTexture);

      this.hasActiveMedia = true;
      this.updateVideoState();
    }).catch((error) => {
      console.error('Error playing video:', error);
    });
  }

  private processInputs() {
    if (!this.seekMode) return;

    // Get the max controller axes.
    const inputState = this.vrInput.getState();
    let axisX = 0.0;
    for (const [gamepad, state] of inputState) {
      if (Math.abs(state.axisX) > Math.abs(axisX)) axisX = state.axisX;
    }

    // If axis is in deadzone, reset seek dir.
    if (Math.abs(axisX) < 0.1) {
      this.hasSeekDir = false;
      return;
    }

    // If a direction is newly pressed, seek.
    if (!this.hasSeekDir && axisX > 0.5) {
      this.hasSeekDir = true;
      this.seekVideo(/*seconds=*/10);
    } else if (!this.hasSeekDir && axisX < -0.5) {
      this.hasSeekDir = true;
      this.seekVideo(/*seconds=*/-10);
    }
  }

  private setVideoPause(paused: boolean) {
    if (!this.videoElement) return;
    if (paused) {
      this.videoElement.pause();
    } else {
      this.videoElement.play();
    }
  }

  private setVideoMute(muted: boolean) {
    if (!this.videoElement) return;
    this.videoElement.muted = muted;
  }

  private seekVideo(seconds: number) {
    if (!this.videoElement) return;
    const video = this.videoElement;

    // Duration may be NaN or Infinity before metadata is available.
    const duration = video.duration;
    if (!Number.isFinite(duration)) return;

    const newTime = video.currentTime + seconds;
    video.currentTime = Math.min(duration, Math.max(0.0, newTime));
  }

  private configureVideoTexture(texture: THREE.Texture) {
    texture.generateMipmaps = false;
    texture.colorSpace = THREE.SRGBColorSpace;
    texture.minFilter = THREE.LinearFilter;
    texture.magFilter = THREE.LinearFilter;
  }

  private updateVideoState() {
    if (!this.hasActiveMedia) return;
    if (!this.mediaInterface || !this.videoElement) return;
    this.mediaInterface.setVideoState({
      duration: this.videoElement.duration,
      currentTime: this.videoElement.currentTime,
      paused: this.videoElement.paused,
      muted: this.videoElement.muted,
    });
  }
}
