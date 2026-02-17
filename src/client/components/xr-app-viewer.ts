import { html, css, LitElement } from 'lit';
import { customElement, property, state } from 'lit/decorators.js';

@customElement('xr-app-viewer')
export class XrAppViewer extends LitElement {
  static styles = css`
    :host {
      display: flex;
      flex-direction: column;
      padding: 16px;
    }
  `;

  @property({ type: String }) private path = '';

  override render() {
    return html`
      <div>Viewer for path: ${this.path}</div>
    `;
  }
}
