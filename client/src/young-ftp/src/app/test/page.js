import * as React from "react"
import fileItemInfo from "../utils/fileItemInfo"
import FileItem from "../components/fileItem"

export default function Test() {
    const info = new fileItemInfo('rwxr-xr-x  2 root root 4096 Dec  1  2019 2022.mp3');
    const data = info.getData();
    console.log(data);
    return (
        <div>
            <FileItem fileItemInfo={data} />
        </div>
    )
}