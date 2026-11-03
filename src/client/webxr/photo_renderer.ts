import { MediaInterface } from './media_interface';
import { VRInput } from './vr_input';
import * as stringUtils from '../modules/string_utils';
import * as THREE from 'three';
import * as types from '../modules/client_types';

// Renderer for left/right eye photos and videos.
export class PhotoRenderer {
  private renderer: THREE.WebGLRenderer;
  private scene: THREE.Scene;
  private textureLoader = new THREE.TextureLoader();
  private eyesGroup = new THREE.Group();

  // The current media in two materials.
  private leftMaterial = new THREE.MeshBasicMaterial();
  private rightMaterial = new THREE.MeshBasicMaterial();

  // The video element and texture to play back video media.
  private videoElement?: HTMLVideoElement;
  private videoTexture?: THREE.VideoTexture;

  // The media interface which is updated every frame.
  private mediaInterface: MediaInterface;

  // In seek mode, left and right directions seek by 10 seconds.
  private seekMode = false;
  private hasSeekDir = false;

  constructor(
    renderer: THREE.WebGLRenderer,
    scene: THREE.Scene,
    mediaInterface: MediaInterface,
  ) {
    this.renderer = renderer;
    this.scene = scene;
    this.setMaterialTexture(this.leftMaterial, null);
    this.setMaterialTexture(this.rightMaterial, null);

    // Create the half-sphere geometry for the left and right eye.
    this.createEyesGeometry();

    // Register media callbacks.
    this.mediaInterface = mediaInterface;
    mediaInterface.addMediaChangedCallback(() => {
      this.changeMedia(mediaInterface.getMedia());
    });
    mediaInterface.addVideoPauseCallback(() => {
      const videoState = mediaInterface.getVideoState();
      if (!videoState) return;
      this.setVideoPause(videoState.paused);
    });
    mediaInterface.addVideoMuteCallback(() => {
      const videoState = mediaInterface.getVideoState();
      if (!videoState) return;
      this.setVideoMute(videoState.muted);
    });
  }

  cleanupResources() {
    // Remove the eyes geometry from the scene.
    this.scene.remove(this.eyesGroup);

    // Clean up old textures.
    this.setMaterialTexture(this.leftMaterial, null);
    this.setMaterialTexture(this.rightMaterial, null);

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
  }

  updateFrame(vrInput: VRInput) {
    this.processInputs(vrInput);
    this.updatePhotoState();
    this.updateVideoState();
  }

  // TODO: This should be removed. Seeking can be done via mediaInterface.
  setSeekMode(seekMode: boolean) {
    this.seekMode = seekMode;
  }

  private changeMedia(media?: types.SelectedMedia) {
    this.cleanupResources();
    if (!media) return;
  
    const album = media.album;
    const entry = album.entries[media.index];
    const isImage = entry.media === types.MediaType.IMAGE;
    const isVideo = entry.media === types.MediaType.VIDEO;
    if (!isImage && !isVideo) return;

    // Add the left/right sphere meshes to the scene.
    this.scene.add(this.eyesGroup);

    if (isImage) {
      this.setSideBySidePhoto(media);
    } else if (isVideo) {
      this.setSideBySideVideo(media);
    }
  }

  private processInputs(vrInput: VRInput) {
    if (!this.seekMode) return;

    // Get the max controller axes.
    const inputState = vrInput.getState();
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

  private createEyesGeometry() {
    const geometry = new THREE.SphereGeometry(
          /*radius=*/75,
          /*widthSegments=*/64,
          /*heightSegments=*/64,
          /*phiStart=*/Math.PI,
          /*phiLength=*/Math.PI  // Half circle.
    );
    // Flip to see the sphere from the inside.
    geometry.scale(-1, 1, 1);

    // Create the two meshes for the left and right eyes. Layers control mesh
    // visibility for the eyes. Default layer is 0, put meshes only on 1 and 2.
    const leftMesh = new THREE.Mesh(geometry, this.leftMaterial);
    leftMesh.layers.set(1);
    this.eyesGroup.add(leftMesh);

    const rightMesh = new THREE.Mesh(geometry, this.rightMaterial);
    rightMesh.layers.set(2);
    this.eyesGroup.add(rightMesh);
  }

  private setSideBySidePhoto(media: types.SelectedMedia) {
    const url = this.getSbsMediaRequest(media);
    this.textureLoader.load(url, (texture) => {
      this.updatePhotoState();
      this.configurePhotoTexture(texture);
      const [left, right] = this.createStereoTextures(texture);
      this.setMaterialTexture(this.leftMaterial, left);
      this.setMaterialTexture(this.rightMaterial, right);
    });
  }

  private setSideBySideVideo(media: types.SelectedMedia) {
    this.videoElement = document.createElement('video');
    this.videoElement.src = this.getSbsMediaRequest(media);
    this.videoElement.crossOrigin = 'anonymous';
    this.videoElement.loop = true;
    this.videoElement.muted = true;
    this.videoElement.playsInline = true;
    this.videoElement.play().then(() => {
      if (!this.videoElement) return;
      this.updateVideoState();
      this.videoTexture = new THREE.VideoTexture(this.videoElement);
      this.configureVideoTexture(this.videoTexture);
      const [left, right] = this.createStereoTextures(this.videoTexture);
      this.setMaterialTexture(this.leftMaterial, left);
      this.setMaterialTexture(this.rightMaterial, right);
    }).catch((error) => {
      console.error('Error playing video:', error);
    });
  }

  private setMaterialTexture(
    material: THREE.MeshBasicMaterial, texture: THREE.Texture | null) {
    material.map?.dispose();  // Avoid GPU leaks.
    material.map = texture;
    material.color = new THREE.Color(texture ? 0xffffff : 0x333333);
    material.needsUpdate = true;
  }

  private configurePhotoTexture(texture: THREE.Texture) {
    texture.generateMipmaps = true;
    texture.colorSpace = THREE.SRGBColorSpace;
    texture.minFilter = THREE.LinearMipMapLinearFilter;
    texture.magFilter = THREE.LinearFilter;
    texture.anisotropy = this.renderer.capabilities.getMaxAnisotropy();
  }

  private configureVideoTexture(texture: THREE.Texture) {
    texture.generateMipmaps = false;
    texture.colorSpace = THREE.SRGBColorSpace;
    texture.minFilter = THREE.LinearFilter;
    texture.magFilter = THREE.LinearFilter;
  }

  private createStereoTextures(texture: THREE.Texture): THREE.Texture[] {
    const leftTexture = texture.clone();
    leftTexture.repeat.set(0.5, 1);
    leftTexture.offset.set(0, 0);

    const rightTexture = texture.clone();
    rightTexture.repeat.set(0.5, 1);
    rightTexture.offset.set(0.5, 0);

    return [leftTexture, rightTexture];
  }

  private getSbsMediaRequest(media: types.SelectedMedia): string {
    const index = media.index;
    const album = media.album;
    const entry = album.entries[index];
    return stringUtils.joinPaths('/media', album.path, entry.name);
  }

  private updatePhotoState() {}

  private updateVideoState() {
    if (!this.mediaInterface || !this.videoElement) return;
    this.mediaInterface.setVideoState({
      duration: this.videoElement.duration,
      currentTime: this.videoElement.currentTime,
      paused: this.videoElement.paused,
      muted: this.videoElement.muted,
    });
  }
}
