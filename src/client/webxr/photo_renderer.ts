import { MediaInterface } from './media_interface';
import { SbsRendererBase } from './sbs_renderer_base';
import { VRInput } from './vr_input';
import * as mediaUtils from '../modules/media_utils';
import * as THREE from 'three';
import * as types from '../modules/client_types';

// Renderer for left/right eye photos and videos.
export class PhotoRenderer extends SbsRendererBase {
  private textureLoader = new THREE.TextureLoader();
  private hasActiveMedia = false;

  constructor(
    renderer: THREE.WebGLRenderer,
    scene: THREE.Scene,
    vrInput: VRInput,
    mediaInterface: MediaInterface,
  ) {
    super(renderer, scene, vrInput);

    mediaInterface.addMediaChangedCallback(() => {
      this.changeMedia(mediaInterface.getMedia());
    });
  }

  override cleanupResources() {
    this.hasActiveMedia = false;
    super.cleanupResources();
  }

  updateFrame() {
    this.updatePhotoState();
  }

  private changeMedia(media?: types.SelectedMedia) {
    this.cleanupResources();
    if (!media || !mediaUtils.isPhotoMedia(media)) return;

    // Load the photo and assign the left/right textures.
    const url = mediaUtils.getMediaUrl(media);
    this.textureLoader.load(url, (texture) => {
      this.configurePhotoTexture(texture);
      this.setSideBySideTexture(texture);

      this.hasActiveMedia = true;
      this.updatePhotoState();
    });
  }

  private configurePhotoTexture(texture: THREE.Texture) {
    texture.generateMipmaps = true;
    texture.colorSpace = THREE.SRGBColorSpace;
    texture.minFilter = THREE.LinearMipMapLinearFilter;
    texture.magFilter = THREE.LinearFilter;
    texture.anisotropy = this.renderer.capabilities.getMaxAnisotropy();
  }

  private updatePhotoState() {
    if (!this.hasActiveMedia) return;
  }
}
