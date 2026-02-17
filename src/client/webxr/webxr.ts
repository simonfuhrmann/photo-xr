import * as THREE from 'three';

class PhotoXR {
  // THREE global objects.
  scene: THREE.Scene;
  renderer: THREE.WebGLRenderer;
  camera: THREE.PerspectiveCamera;
  textureLoader: THREE.TextureLoader;

  // Test geometry.
  plane1: THREE.Mesh;
  plane2: THREE.Mesh;


  constructor() {
    this.scene = new THREE.Scene();
    this.scene.background = new THREE.Color(0x000000);

    // Create a camera. The FOV and aspect ratio is overridden by WebXR.
    const aspect = window.innerWidth / window.innerHeight;
    const zNear = 0.1;
    const zFar = 100.0;
    this.camera = new THREE.PerspectiveCamera(/*fov=*/70, aspect, zNear, zFar);

    // Enable the hemisphere layers for this camera (default is 0 only).
    // TODO: Maybe move into render loop.
    this.camera.layers.enable(1);
    this.camera.layers.enable(2);

    // Set up the renderer and enable WebXR. Disable foveation since it causes
    // blurriness at the bottom of the rendering. The framebuffer scale of 2.0
    // is expensive, but I imagine to see a tiny difference. Need better test
    // images to see the difference.
    this.renderer = new THREE.WebGLRenderer({ antialias: true });
    this.renderer.setSize(window.innerWidth, window.innerHeight);
    this.renderer.xr.enabled = true;
    this.renderer.xr.setFoveation(0);
    this.renderer.xr.setFramebufferScaleFactor(2.0); // Test more.
    document.body.appendChild(this.renderer.domElement);

    // Create the texture loader.
    this.textureLoader = new THREE.TextureLoader();

    // Simple test geometry so we see something in VR.
    {
      const photoFilename = '/photo/deovr/burari/burari_vr01_3840p.jpg';
      const texture = this.textureLoader.load(photoFilename);
      texture.colorSpace = THREE.SRGBColorSpace;
      texture.minFilter = THREE.LinearFilter;

      const leftTexture = texture.clone();
      leftTexture.repeat.set(0.5, 1);
      leftTexture.offset.set(0, 0);

      const rightTexture = texture.clone();
      rightTexture.repeat.set(0.5, 1);
      rightTexture.offset.set(0.5, 0);

      const geometry = new THREE.PlaneGeometry(1, 1); // adjust ratio

      this.plane1 = new THREE.Mesh(geometry, new THREE.MeshBasicMaterial({ map: leftTexture }));
      this.plane1.position.y = 1.5;
      this.plane1.position.z = -1;
      this.plane1.position.x = -0.27;
      this.plane1.scale.setScalar(0.5);
      // this.scene.add(this.plane1);

      this.plane2 = new THREE.Mesh(geometry, new THREE.MeshBasicMaterial({ map: rightTexture }));
      this.plane2.position.y = 1.5;
      this.plane2.position.z = -1;
      this.plane2.position.x = 0.27;
      this.plane2.scale.setScalar(0.5);
      // this.scene.add(this.plane2);
    }

    {
      const geometry = new THREE.SphereGeometry(
        75,        // radius
        64,         // width segments
        64,         // height segments
        Math.PI, // phiStart
        Math.PI     // phiLength (180°)
      );
      geometry.scale(-1, 1, 1);  // Flip so we view from inside
      const photoFilename = '/photo/deovr/burari/burari_vr01_3840p.jpg';
      const texture = this.textureLoader.load(photoFilename);
      texture.generateMipmaps = false;
      texture.colorSpace = THREE.SRGBColorSpace;
      texture.minFilter = THREE.LinearFilter;
      texture.magFilter = THREE.LinearFilter;
      texture.anisotropy = this.renderer.capabilities.getMaxAnisotropy();

      const leftTexture = texture.clone();
      leftTexture.repeat.set(0.5, 1);
      leftTexture.offset.set(0, 0);

      const rightTexture = texture.clone();
      rightTexture.repeat.set(0.5, 1);
      rightTexture.offset.set(0.5, 0);

      const leftMaterial = new THREE.MeshBasicMaterial({ map: leftTexture });
      const rightMaterial = new THREE.MeshBasicMaterial({ map: rightTexture });

      const leftMesh = new THREE.Mesh(geometry, leftMaterial);
      const rightMesh = new THREE.Mesh(geometry, rightMaterial);
      // Default layer is 0. Keep hemispheres only on 1 and 2.
      leftMesh.layers.set(1);
      rightMesh.layers.set(2);

      this.scene.add(leftMesh);
      this.scene.add(rightMesh);
    }
  }

  bindSession(session: XRSession) {
    this.renderer.xr.setSession(session);
    this.renderer.setAnimationLoop(() => {

      const xrCamera = this.renderer.xr.getCamera();
      this.camera.layers.enable(1);
      this.camera.layers.enable(2);

      if (xrCamera.cameras.length === 2) {
        xrCamera.cameras[0].layers.set(1); // left eye
        xrCamera.cameras[1].layers.set(2); // right eye
      }

      // Layer 0 is for the left eye, layer 1 is for the right eye.
      // xrCamera.cameras[0].layers.enable(1);
      // xrCamera.cameras[0].layers.disable(2);
      // xrCamera.cameras[1].layers.enable(2);
      // xrCamera.cameras[1].layers.disable(1);
      this.renderer.render(this.scene, this.camera);
    });

    session.addEventListener('end', () => {
      this.renderer.setAnimationLoop(null);
      this.renderer.domElement.remove();
    });
  }

  endSession() {
    const session = this.renderer.xr.getSession();
    if (!session) return;
    session.end();
  }
}

let photoXR: PhotoXR | undefined;

// Initializes WebXR and starts the session.
export function startSession(session: XRSession) {
  if (!!photoXR) return;
  photoXR = new PhotoXR();
  photoXR.bindSession(session);
}

// Ends the WebXR session.
export function endSession() {
  if (!photoXR) return;
  photoXR.endSession();
  photoXR = undefined;
}
