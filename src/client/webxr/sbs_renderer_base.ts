import { VRInput } from './vr_input';
import * as THREE from 'three';

export class SbsRendererBase {
  protected renderer: THREE.WebGLRenderer;
  protected scene: THREE.Scene;

  // The current media in two materials.
  protected leftMaterial = new THREE.MeshBasicMaterial();
  protected rightMaterial = new THREE.MeshBasicMaterial();

  // Group where the meshes for the two eyes are stored in.
  private eyesGroup = new THREE.Group();

  // The interface for receiving user inputs.
  protected vrInput: VRInput;

  constructor(
    renderer: THREE.WebGLRenderer,
    scene: THREE.Scene,
    vrInput: VRInput,
  ) {
    this.renderer = renderer;
    this.scene = scene;
    this.vrInput = vrInput;

    // Initialize the textures with a dark gray material.
    this.setMaterialTexture(this.leftMaterial, null);
    this.setMaterialTexture(this.rightMaterial, null);

    // Create the half-sphere geometry for the left and right eye.
    this.createEyesGeometry();
  }

  protected cleanupResources() {
    this.scene.remove(this.eyesGroup);

    // Clean up the texture maps on the textures.
    this.setMaterialTexture(this.leftMaterial, null);
    this.setMaterialTexture(this.rightMaterial, null);
  }

  protected setSideBySideTexture(texture: THREE.Texture) {
    // Create two textures with left/right views on the side by side texture.
    const leftTexture = texture.clone();
    leftTexture.repeat.set(0.5, 1);
    leftTexture.offset.set(0, 0);

    const rightTexture = texture.clone();
    rightTexture.repeat.set(0.5, 1);
    rightTexture.offset.set(0.5, 0);

    // Assign the left/right textures to the materials.
    this.setMaterialTexture(this.leftMaterial, leftTexture);
    this.setMaterialTexture(this.rightMaterial, rightTexture);

    // Ensure the meshes for the eyes are added to the scene.
    this.scene.remove(this.eyesGroup);
    this.scene.add(this.eyesGroup);
  }

  private setMaterialTexture(
    material: THREE.MeshBasicMaterial,
    texture: THREE.Texture | null,
  ) {
    material.map?.dispose();  // Avoid GPU leaks.
    material.map = texture;
    material.color = new THREE.Color(texture ? 0xffffff : 0x333333);
    material.needsUpdate = true;
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

}
