import * as THREE from 'three';
import * as types from '../modules/client_types';
import * as stringUtils from '../modules/string_utils';

export class MediaViewer {
  private scene: THREE.Scene;
  private renderer: THREE.WebGLRenderer;
  private textureLoader: THREE.TextureLoader = new THREE.TextureLoader();

  // The currently selected media.
  private media?: types.SelectedMedia;

  // The current media in two materials.
  private leftMaterial = new THREE.MeshBasicMaterial();
  private rightMaterial = new THREE.MeshBasicMaterial();

  // The video element and texture to play back video media.
  private videoElement?: HTMLVideoElement;
  private videoTexture?: THREE.VideoTexture;

  constructor(renderer: THREE.WebGLRenderer, scene: THREE.Scene) {
    this.scene = scene;
    this.renderer = renderer;
    this.setMaterialTexture(this.leftMaterial, null);
    this.setMaterialTexture(this.rightMaterial, null);

    // Create the half-sphere geometry for the left and right eye.
    this.createEyesGeometry();
  }

  changeMedia(media?: types.SelectedMedia) {
    this.cleanupResources();
    this.media = media;
    if (!this.media) return;

    const album = this.media.album;
    const entry = album.entries[this.media.index];
    if (entry.type === types.EntryType.PHOTO) {
      this.setSideBySidePhoto(this.media);
    } else if (entry.type === types.EntryType.VIDEO) {
      this.setSideBySideVideo(this.media);
    }
  }

  toggleVideoPause() {
    if (!this.videoElement) return;
    if (this.videoElement.paused) {
      this.videoElement.play();
    } else {
      this.videoElement.pause();
    }
  }

  toggleVideoMute() {
    if (!this.videoElement) return;
    this.videoElement.muted = !this.videoElement.muted;
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
    this.scene.add(leftMesh);

    const rightMesh = new THREE.Mesh(geometry, this.rightMaterial);
    rightMesh.layers.set(2);
    this.scene.add(rightMesh);
  }

  private setSideBySidePhoto(media: types.SelectedMedia) {
    const url = this.getSbsMediaRequest(media);
    this.textureLoader.load(url, (texture) => {
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
    material: THREE.MeshBasicMaterial, texture: THREE.Texture|null) {
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

  private cleanupResources() {
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
}
