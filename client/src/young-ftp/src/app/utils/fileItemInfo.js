class fileItemInfo {
  constructor(lsString) {
    const lsArray = lsString.split(/\s+/);
    this.access = lsArray[0];
    this.owner = lsArray[2];
    this.group = lsArray[3];
    this.size = lsArray[4];
    this.lastModified = lsArray[5] + ' ' + lsArray[6] + ' ' + lsArray[7];
    this.name = lsArray.slice(8).join(' ');
    this.isDir = lsArray[0].charAt(0) === 'd';
    this.type = this.isDir ? 'directory' : this.getFileType(this.name);
  }

  getFileType(fileName) {
    const dotIndex = fileName.lastIndexOf('.');
    if (dotIndex !== -1) {
      return fileName.substring(dotIndex + 1);
    }
    return 'binary';
  }

  getData() {
    return {
      access: this.access,
      owner: this.owner,
      group: this.group,
      size: this.size,
      lastModified: this.lastModified,
      name: this.name,
      isDir: this.isDir,
      type: this.type
    };
  }
}

export default fileItemInfo;