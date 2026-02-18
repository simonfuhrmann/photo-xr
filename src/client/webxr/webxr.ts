import * as THREE from 'three';
import * as types from '../modules/client_types';
import { VRInput } from './vr_input';

type ButtonEvent = { source: XRInputSource, button: number, pressed: boolean };

class PhotoXR {
  // The album and media to display in this session.
  media?: types.SelectedMedia;

  // THREE global objects.
  scene: THREE.Scene;
  renderer: THREE.WebGLRenderer;
  camera: THREE.PerspectiveCamera;
  textureLoader: THREE.TextureLoader = new THREE.TextureLoader();

  // The current photo in two materials.
  leftMaterial = new THREE.MeshBasicMaterial({ color: 0x333333 });
  rightMaterial = new THREE.MeshBasicMaterial({ color: 0x333333 });

  // VR input handling.
  vrInput: VRInput;

  constructor() {
    this.scene = new THREE.Scene();
    this.scene.background = new THREE.Color(0x000000);

    // Create a camera. The FOV and aspect ratio is overridden by WebXR.
    const aspect = window.innerWidth / window.innerHeight;
    const zNear = 0.1;
    const zFar = 100.0;
    this.camera = new THREE.PerspectiveCamera(/*fov=*/70, aspect, zNear, zFar);

    // Enable the hemisphere layers for this camera (default is 0 only).
    this.camera.layers.enable(1);
    this.camera.layers.enable(2);

    // Set up the renderer and enable WebXR. Disable foveation since it causes
    // blurriness at the bottom of the rendering. Increasing the framebuffer
    // scale is expensive, but I imagine to see a tiny difference. Need better
    // test images to see the difference.
    this.renderer = new THREE.WebGLRenderer({ antialias: false });
    this.renderer.setSize(window.innerWidth, window.innerHeight);
    this.renderer.xr.enabled = true;
    this.renderer.xr.setFoveation(0);
    this.renderer.xr.setFramebufferScaleFactor(1.5); // Test more.

    // Create the VR input infrastructure.
    this.vrInput = new VRInput(this.renderer, this.scene);
    //this.vrInput.addControllerModels();
    this.vrInput.addEventListener('select', this.onSelect.bind(this));
    this.vrInput.addEventListener('squeeze', this.onSqueeze.bind(this));
    this.vrInput.addEventListener('button', this.onButton.bind(this));

    // Create the half-sphere geometry for the left and right eye.
    this.createEyesGeometry();
  }

  setMedia(media: types.SelectedMedia) {
    this.media = media;
    this.onChangePhoto(0);
  }

  bindSession(session: XRSession) {
    this.renderer.xr.setSession(session);
    this.renderer.setAnimationLoop(() => {
      const xrCameras = this.renderer.xr.getCamera();
      if (xrCameras.cameras.length === 2) {
        xrCameras.cameras[0].layers.set(1); // left eye
        xrCameras.cameras[1].layers.set(2); // right eye
      }
      this.vrInput.pollInputs();
      this.renderer.render(this.scene, this.camera);
    });

    session.addEventListener('end', () => {
      this.renderer.xr.setSession(null);
      this.renderer.setAnimationLoop(null);
      this.renderer.dispose();
      console.log('XR session ended');
    });
  }

  endSession() {
    this.renderer.xr.getSession()?.end();
  }

  private createEyesGeometry() {
    const geometry = new THREE.SphereGeometry(
      /*radius=*/75,
      /*widthSegments=*/64,
      /*heightSegments=*/64,
      /*phiStart=*/Math.PI,
      /*phiLength=*/Math.PI  // Half circle.
    );
    geometry.scale(-1, 1, 1);  // Flip so we view from inside

    const leftMesh = new THREE.Mesh(geometry, this.leftMaterial);
    const rightMesh = new THREE.Mesh(geometry, this.rightMaterial);

    // Default layer is 0. Keep hemispheres only on 1 and 2.
    leftMesh.layers.set(1);
    rightMesh.layers.set(2);

    this.scene.add(leftMesh);
    this.scene.add(rightMesh);
  }

  private createStereoTextures(texture: THREE.Texture) {
    const leftTexture = texture.clone();
    leftTexture.repeat.set(0.5, 1);
    leftTexture.offset.set(0, 0);

    const rightTexture = texture.clone();
    rightTexture.repeat.set(0.5, 1);
    rightTexture.offset.set(0.5, 0);

    return { leftTexture, rightTexture };
  }

  private getMediaRequest(media: types.SelectedMedia) {
    const index = media.index;
    const album = media.album;
    const entry = album.entries[index];
    return `/photo/${album.path}/${entry.name}`;
  }

  private onSelect(event: Event) {
    // This will bring up the immersive UI.
    console.log('select event');
  }

  private onSqueeze(event: Event) {
    console.log('squeeze event', event);
  }

  private onButton(event: Event) {
    const ev = event as CustomEvent<ButtonEvent>;
    const buttonIndex = ev.detail.button;
    const pressed = ev.detail.pressed;
    if (!pressed) return;
    if (buttonIndex === 4) this.onChangePhoto(1);
    if (buttonIndex === 5) this.onChangePhoto(-1);
  }

  private onChangePhoto(delta: number) {
    if (!this.media) return;

    // Advance the media index.
    const album = this.media.album;
    const count = album.entries.length;
    this.media.index = (this.media.index + delta + count) % count;

    // Load the new photo.
    const url = this.getMediaRequest(this.media);
    this.textureLoader.load(url, (texture) => {
      texture.generateMipmaps = true;
      texture.colorSpace = THREE.SRGBColorSpace;
      texture.minFilter = THREE.LinearMipMapLinearFilter;
      texture.magFilter = THREE.LinearFilter;
      texture.anisotropy = this.renderer.capabilities.getMaxAnisotropy();

      const { leftTexture, rightTexture } =
        this.createStereoTextures(texture);

      // Dispose old textures to avoid GPU leaks and update to new textures.
      this.leftMaterial.map?.dispose();
      this.rightMaterial.map?.dispose();
      this.leftMaterial.map = leftTexture;
      this.rightMaterial.map = rightTexture;
      this.leftMaterial.color = new THREE.Color(0xffffff);
      this.rightMaterial.color = new THREE.Color(0xffffff);
      this.leftMaterial.needsUpdate = true;
      this.rightMaterial.needsUpdate = true;
    });
  }
}

// Singleton instance, created and disposed on demand.
let photoXR: PhotoXR | undefined;

// Initializes WebXR and starts the session.
export function startSession(session: XRSession, media: types.SelectedMedia) {
  if (!!photoXR) return;
  photoXR = new PhotoXR();
  photoXR.bindSession(session);
  photoXR.setMedia(media);
  console.log('XR session started');
}

// Ends the WebXR session.
export function endSession() {
  if (!photoXR) return;
  photoXR.endSession();
  photoXR = undefined;
}

export function updateMedia(media: types.SelectedMedia) {
  if (!photoXR) return;
  photoXR.setMedia(media);
}
